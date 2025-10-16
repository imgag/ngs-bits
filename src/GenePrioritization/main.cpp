#include "ToolBase.h"
#include "Graph.h"
#include "Helper.h"
#include <cmath>
#include <QTextStream>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QList>
#include <QListIterator>
#include <QHash>
#include <QSharedPointer>
#include <random>

struct NodeContent
{
    double score;
    double score_change;
    int visit_count;

    NodeContent()
        : score(0.0),
          score_change(0.0),
          visit_count(0)
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
    QList<QString> starting_nodes_;

    void sortGenesByScore(QList<Graph<NodeContent, EdgeContent>::NodePointer>& node_list)
    {
        // round score for each node to 6 decimal places
        Graph<NodeContent, EdgeContent>::NodePointer node;
        foreach(node, node_list)
        {
            node.data()->nodeContent().score = round(node.data()->nodeContent().score * 1e6) / 1e6;
        }

        std::sort(node_list.begin(), node_list.end(),\
                  [](const Graph<NodeContent, EdgeContent>::NodePointer& a, const Graph<NodeContent, EdgeContent>::NodePointer& b)\
                  {
            if(a.data()->nodeContent().score == b.data()->nodeContent().score)
            {
                return a.data()->nodeName() < b.data()->nodeName();
            }
            return a.data()->nodeContent().score > b.data()->nodeContent().score;});
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

    Graph<NodeContent, EdgeContent>::NodePointer updateProbability(int steps, const Graph<NodeContent, EdgeContent>::NodePointer& node)
    {
        node.data()->nodeContent().score = node.data()->nodeContent().score_change;
        node.data()->nodeContent().score_change = (double) node.data()->nodeContent().visit_count / steps;
        return node;
    }

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
		setDescription("Performs gene prioritization based on list of known disease genes of a disease and a PPI graph.");
		addInfile("in", "Input TSV file with one gene identifier per line (known disease genes of a disease).", false);
		addInfile("graph", "Graph TSV file with two gene identifiers per line (PPI graph).", false);
		addOutfile("out", "Output TSV file containing prioritized genes for the disease.", false);
        //optional
		addEnum("method", "Gene prioritization method to use.", true, QStringList() << "flooding" << "random_walk", "flooding");
		addInt("n", "Number of network diffusion iterations (flooding).", true, 2);
		addFloat("restart", "Restart probability (random_walk).", true, 0.4);
		addOutfile("debug", "Output TSV file for debugging", true);
    }

    Graph<NodeContent, EdgeContent> parseGraph(const QString& graph_file)
    {

        Graph<NodeContent, EdgeContent> graph;

        QSharedPointer<QFile> reader = Helper::openFileForReading(graph_file);
        QTextStream in(reader.data());

        while(!in.atEnd())
        {
            QStringList line = in.readLine().split("\t", QT_SKIP_EMPTY_PARTS);
            if(line.size() == 2)
            {
                NodeContent node_content_1{};
                NodeContent node_content_2{};
                EdgeContent edge_content{};
                edge_content.weight = 1.0;
                graph.addEdge(line.at(0), node_content_1,\
                              line.at(1), node_content_2, edge_content);
            }
        }
        return graph;
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
            QStringList line = in.readLine().split("\t", QT_SKIP_EMPTY_PARTS);

            if(graph.hasNode(line.at(0)))
            {
                graph.getNode(line.at(0)).data()->nodeContent().score = 1.0;
                starting_nodes_.append(line.at(0));
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

            stream << "iteration\taverage_rank_change\tstart_at_top" << QT_ENDL;

            QList<Graph<NodeContent, EdgeContent>::NodePointer> node_list = graph.adjacencyList().keys();
            sortGenesByScore(node_list);
            for(int i = 0; i < node_list.length(); i++)
            {
                previous_ranks.insert(node_list.at(i).data()->nodeName(), i+1);
            }

            stream << 0 << "\tNaN\t" << getStartGenesAtTop(node_list) << QT_ENDL;
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
                            edge.data()->node2().data()->nodeContent().score_change += increment;
                        }
                        else if(edge.data()->node2() == node)
                        {
                            edge.data()->node1().data()->nodeContent().score_change += increment;
                        }
                    }

                    // add the value that was propagated to surrounding nodes to the node itself
                    // node.data()->nodeContent().score_increment += node_score;
                }
            }

            // add the score increment to the node scores, relative to target node degree; reset increments
            foreach (node, graph.adjacencyList().keys())
            {
                node.data()->nodeContent().score += node.data()->nodeContent().score_change / sqrt(graph.getDegree(node.data()->nodeName()));
                node.data()->nodeContent().score_change = 0.0;
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
                       << "\t" << getStartGenesAtTop(node_list) << QT_ENDL;
                previous_ranks = current_ranks;
            }
        }
    }

    void randomWalk(Graph<NodeContent, EdgeContent>& graph, double restart_probability, const QString& debug_file, int max_steps = 1000000)
    {
        std::default_random_engine generator;
        std::uniform_real_distribution<double> restart_distrib(0.0,1.0);

        std::uniform_int_distribution<int> start_nodes_distrib(0, starting_nodes_.size() - 1);

        Graph<NodeContent, EdgeContent>::NodePointer current_node = graph.getNode(starting_nodes_.at(start_nodes_distrib(generator)));
        current_node.data()->nodeContent().visit_count++;
        current_node.data()->nodeContent().score_change = current_node.data()->nodeContent().visit_count;

        int steps{1};
        double vector_diff{1.0};
        int update_frequency{50000};

        bool debug = (debug_file != "");

        QSharedPointer<QFile> writer;
        QTextStream stream;

        // generate debug file with information about difference between probability vectors at each step
        if(debug)
        {
            writer = Helper::openFileForWriting(debug_file);
            stream.setDevice(writer.data());

            stream << "step\tprobability_diff_norm" << QT_ENDL;
            stream << steps << "\tNaN" << QT_ENDL;
        }

        QTextStream out(stdout);

        do
        {
            steps++;

            if(restart_distrib(generator) < restart_probability)
            {
                current_node = graph.getNode(starting_nodes_.at(start_nodes_distrib(generator)));
            }
            else
            {
                QList<Graph<NodeContent, EdgeContent>::NodePointer> adjacent_nodes = graph.getAdjacentNodes(current_node.data()->nodeName());
                std::uniform_int_distribution<int> adjacency_distrib(0, adjacent_nodes.size() - 1);
                current_node = adjacent_nodes.at(adjacency_distrib(generator));
            }
            current_node.data()->nodeContent().visit_count++;

            // update probabilities and calculate vector difference (L1 norm)

            if(steps % update_frequency == 0)
            {
                vector_diff = 0.0;

                QListIterator<Graph<NodeContent, EdgeContent>::NodePointer> iter(graph.adjacencyList().keys());
                while(iter.hasNext())
                {
                    Graph<NodeContent, EdgeContent>::NodePointer node = iter.next();
                    node.data()->nodeContent().score = node.data()->nodeContent().score_change;
                    node.data()->nodeContent().score_change = (double) node.data()->nodeContent().visit_count / steps;
                    vector_diff += fabs(node.data()->nodeContent().score_change - node.data()->nodeContent().score);
                }

                vector_diff /= update_frequency;

                if(debug)
                {
                    stream << steps << "\t" << vector_diff << QT_ENDL;
                    out << steps << "\t" << vector_diff << QT_ENDL;
                }
            }

        } while((vector_diff > 1.0e-6) && (steps < max_steps));

        // obtain final score with penalization of high degrees
        QListIterator<Graph<NodeContent, EdgeContent>::NodePointer> iter(graph.adjacencyList().keys());
        while(iter.hasNext())
        {
            Graph<NodeContent, EdgeContent>::NodePointer node = iter.next();
            node.data()->nodeContent().score = node.data()->nodeContent().visit_count / sqrt(graph.getDegree(node.data()->nodeName()));
        }
    }

    void writeOutputTsv(Graph<NodeContent, EdgeContent>& graph, QString out_file)
    {
        // output all nodes that have a score unequal zero to the output file
        QSharedPointer<QFile> writer = Helper::openFileForWriting(out_file);
        QTextStream stream(writer.data());

        stream << "node\tscore\tstarting_node\tdegree" << QT_ENDL;

        QList<Graph<NodeContent, EdgeContent>::NodePointer> node_list = graph.adjacencyList().keys();
        sortGenesByScore(node_list);

        Graph<NodeContent, EdgeContent>::NodePointer node;
        foreach(node, node_list)
        {
            stream << node.data()->nodeName() << "\t" << node.data()->nodeContent().score\
                   << "\t" << starting_nodes_.contains(node.data()->nodeName()) \
                   << "\t" << graph.getDegree(node.data()->nodeName()) << QT_ENDL;
        }
    }

    virtual void main()
    {
        // init
        QString method = getEnum("method");
        Graph<NodeContent, EdgeContent> interaction_network = parseGraph(getInfile("graph"));

        scoreDiseaseGenes(interaction_network, getInfile("in"));

		if(method == "random_walk")
        {
            randomWalk(interaction_network, getFloat("restart"), getOutfile("debug"));
        }
        else if(method == "flooding")
        {
            performFlooding(interaction_network, getInt("n"), getOutfile("debug"));
        }

        writeOutputTsv(interaction_network, getOutfile("out"));
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

