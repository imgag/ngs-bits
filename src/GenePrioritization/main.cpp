#include "ToolBase.h"
#include "Graph.h"
#include "StringDbParser.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QTextStream>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QList>
#include <QSet>
#include <QHash>
#include <QSharedPointer>

struct NodeContent
{
    double score;
    double score_increment;

    NodeContent()
        : score(0.0),
          score_increment(0.0)
    {
    }
};

struct EdgeContent
{
    double weight;

    EdgeContent()
        : weight(0.0)
    {
    }
};

class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

private:
    QSet<QString> starting_nodes_;

    void sortGenesByScore(QList<Graph<NodeContent, EdgeContent>::NodePointer>& node_list)
    {
        std::sort(node_list.begin(), node_list.end(),\
                  [](const Graph<NodeContent, EdgeContent>::NodePointer& a, const Graph<NodeContent, EdgeContent>::NodePointer& b)\
                  {return a.data()->nodeContent().score > b.data()->nodeContent().score;});
    }

    double getStartGenesAtTop(QList<Graph<NodeContent, EdgeContent>::NodePointer>& node_list)
    {
        int counter{0};
        for(int i = 0; i < starting_nodes_.size(); i++)
        {
            if(starting_nodes_.contains(node_list.at(i).data()->nodeName()))
            {
                counter++;
            }
        }
        return (double) counter / starting_nodes_.size();
    }

    double getAverageRankDifference(const QHash<QString, int>& previous_ranks, const QHash<QString, int>& current_ranks)
    {
        double average_rank_diff{0.0};
        QHashIterator<QString, int> it(current_ranks);
        while(it.hasNext())
        {
            it.next();
            average_rank_diff += abs((it.value() - previous_ranks[it.key()]));
        }
        return average_rank_diff / current_ranks.size();
    }

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
        setDescription("Performs gene prioritization based on list of known disease genes and String-DB.");
        addInfile("in", "Input tsv file with one gene (HGNC identifier) and its disease score per line.", false);
        addInfile("string", "String-DB file with protein interactions", false);
        addInfile("alias", "Input tsv file with aliases for String protein IDs", false);
        addOutfile("out", "Output tsv file with prioritized genes.", false);
        //optional
        addInt("n", "Number of network diffusion iterations", true, 3);
        addFloat("conf", "Confidence score for String-DB interaction; between 0 and 1.", true, 0.4);
        addOutfile("debug-iterations", "Output tsv file for debugging number of iterations", true);
    }

    void scoreDiseaseGenes(Graph<NodeContent, EdgeContent>& graph, QString disease_genes_file)
    {
        QFile file(disease_genes_file);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }

        QTextStream in(&file);

        // read file line by line, changing node content of the disease genes
        while(!in.atEnd())
        {
            QStringList line = in.readLine().split("\t", QString::SkipEmptyParts);

            if(line.size() == 2)
            {
                if(graph.hasNode(line.at(0)))
                {
                    graph.getNode(line.at(0)).data()->nodeContent().score = line.at(1).toDouble();
                    starting_nodes_.insert(line.at(0));
                }
            }
        }
    }

    void performFlooding(Graph<NodeContent, EdgeContent>& graph, int n_iter, const QString& debug_file)
    {
        bool debug = (debug_file != "");

        QSharedPointer<QFile> writer;
        QTextStream stream;

        QHash<QString, int> previous_ranks;

        // generate debug file with information about rank differences between iterations
        if(debug)
        {
            writer = Helper::openFileForWriting(debug_file);
            stream.setDevice(writer.data());

            stream << "iteration\taverage_rank_change\tstart_at_top" << endl;

            QList<Graph<NodeContent, EdgeContent>::NodePointer> node_list = graph.adjacencyList().keys();
            sortGenesByScore(node_list);
            for(int i = 0; i < node_list.length(); i++)
            {
                previous_ranks.insert(node_list.at(i).data()->nodeName(), i+1);
            }

            stream << 0 << "\tNaN\t" << getStartGenesAtTop(node_list) << endl;
        }

        // perform flooding algorithm
        for(int i = 0; i < n_iter; i++)
        {
            Graph<NodeContent, EdgeContent>::NodePointer node;
            foreach(node, graph.adjacencyList().keys())
            {
                double node_score = node.data()->nodeContent().score;

                if(node_score != 0.0)
                {
                    QList<Graph<NodeContent, EdgeContent>::EdgePointer> adjacent_edges = graph.adjacencyList()[node];
                    Graph<NodeContent, EdgeContent>::EdgePointer edge;

                    int degree = graph.getDegree(node.data()->nodeName());

                    // propagate the score to all adjacent nodes, relative to node degree
                    foreach(edge, adjacent_edges)
                    {
                        double increment = node_score / degree;

                        if(edge.data()->node1() == node)
                        {
                            edge.data()->node2().data()->nodeContent().score_increment += increment;
                        }
                        else if(edge.data()->node2() == node)
                        {
                            edge.data()->node1().data()->nodeContent().score_increment += increment;
                        }
                    }

                    // add the value that was propagated to surrounding nodes to the node itself
                    // node.data()->nodeContent().score_increment += node_score;
                }
            }

            // add the score increment to the node scores, relative to target node degree; reset increments
            foreach (node, graph.adjacencyList().keys())
            {
                node.data()->nodeContent().score += node.data()->nodeContent().score_increment / sqrt(graph.getDegree(node.data()->nodeName()));
                node.data()->nodeContent().score_increment = 0.0;
            }

            // write average rank difference to debug file
            if(debug)
            {
                QList<Graph<NodeContent, EdgeContent>::NodePointer> node_list = graph.adjacencyList().keys();
                sortGenesByScore(node_list);
                QHash<QString, int> current_ranks;
                for(int j = 0; j < node_list.length(); j++)
                {
                    current_ranks.insert(node_list.at(j).data()->nodeName(), j+1);
                }

                stream << i+1 << "\t" << getAverageRankDifference(current_ranks, previous_ranks) \
                       << "\t" << getStartGenesAtTop(node_list) << endl;
                previous_ranks = current_ranks;
            }
        }
    }

    void writeOutputTsv(Graph<NodeContent, EdgeContent>& graph, QString out_file)
    {
        // output all nodes that have a score unequal zero to the output file
        QSharedPointer<QFile> writer = Helper::openFileForWriting(out_file);
        QTextStream stream(writer.data());

        stream << "node\tscore\tstarting_node\tdegree" << endl;

        QList<Graph<NodeContent, EdgeContent>::NodePointer> node_list = graph.adjacencyList().keys();
        sortGenesByScore(node_list);

        Graph<NodeContent, EdgeContent>::NodePointer node;
        foreach(node, node_list)
        {
            if(node.data()->nodeContent().score != 0.0)
            {
                stream << node.data()->nodeName() << "\t" << node.data()->nodeContent().score\
                       << "\t" << starting_nodes_.contains(node.data()->nodeName()) \
                       << "\t" << graph.getDegree(node.data()->nodeName()) << endl;
            }
            else
            {
                break;
            }
        }
    }

    virtual void main()
    {
        // init
        StringDbParser<NodeContent, EdgeContent> string_parser(getInfile("string"), getInfile("alias"), getFloat("conf"));
        Graph<NodeContent, EdgeContent> interaction_network = string_parser.interactionNetwork();

        scoreDiseaseGenes(interaction_network, getInfile("in"));

        performFlooding(interaction_network, getInt("n"), getOutfile("debug-iterations"));

        writeOutputTsv(interaction_network, getOutfile("out"));
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

