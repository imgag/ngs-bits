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

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
        setDescription("Performs gene prioritization based on list of known disease genes and String-DB.");
        addInfile("in", "Input tsv file with one gene and its disease score per line.", false);
        addInfile("string", "String-DB file with protein interactions", false);
        addInfile("alias", "Input tsv file with aliases for String protein IDs", false);
        addOutfile("out", "Output tsv file with prioritized genes.", false);
        //optional
        addFloat("conf", "Confidence score for String-DB interaction; between 0 and 1.", true, 0.4);
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
                }
            }
        }
    }

    void performFlooding(Graph<NodeContent, EdgeContent>& graph, int n_iter)
    {
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

                    // calculate the sum and max of all weights of edges adjacent to the current node
                    double edge_weight_sum = 0.0;
                    double edge_weight_max = 0.0;

                    foreach(edge, adjacent_edges)
                    {
                        double edge_weight = edge.data()->edgeContent().weight;
                        edge_weight_sum += edge_weight;
                        if(edge_weight > edge_weight_max)
                        {
                            edge_weight_max = edge_weight;
                        }
                    }

                    // propagate the score to all adjacent nodes, according to edge weight
                    foreach(edge, adjacent_edges)
                    {
                        double increment = node_score * (edge.data()->edgeContent().weight / edge_weight_sum);

                        if(edge.data()->node1() == node)
                        {
                            edge.data()->node2().data()->nodeContent().score_increment += increment;
                        }
                        else if(edge.data()->node2() == node)
                        {
                            edge.data()->node1().data()->nodeContent().score_increment += increment;
                        }
                    }

                    // add the maximum that was propagated to surrounding nodes to the node itself
                    node.data()->nodeContent().score_increment += node_score * (edge_weight_max / edge_weight_sum);
                }
            }

            // add the score increment to the node scores; reset increments
            foreach (node, graph.adjacencyList().keys())
            {
                node.data()->nodeContent().score += node.data()->nodeContent().score_increment;
                node.data()->nodeContent().score_increment = 0.0;
            }
        }
    }

    void writeOutputTsv(Graph<NodeContent, EdgeContent>& graph, QString out_file)
    {
        // output all nodes that have a score unequal zero to the output file
        QSharedPointer<QFile> writer = Helper::openFileForWriting(out_file);
        QTextStream stream(writer.data());

        stream << "node\tscore" << endl;

        QList<Graph<NodeContent, EdgeContent>::NodePointer> node_list = graph.adjacencyList().keys();
        std::sort(node_list.begin(), node_list.end(),\
                  [](const Graph<NodeContent, EdgeContent>::NodePointer& a, const Graph<NodeContent, EdgeContent>::NodePointer& b)\
                  {return a.data()->nodeContent().score > b.data()->nodeContent().score;});

        Graph<NodeContent, EdgeContent>::NodePointer node;
        foreach(node, node_list)
        {
            if(node.data()->nodeContent().score != 0.0)
            {
                stream << node.data()->nodeName() << "\t" << node.data()->nodeContent().score << endl;
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

        performFlooding(interaction_network, 3);

        writeOutputTsv(interaction_network, getOutfile("out"));
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

