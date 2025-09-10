#include "TestFramework.h"
#include "Graph.h"
#include "Helper.h"
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QSharedPointer>

struct EdgeContent
{
    double weight;

    EdgeContent()
        : weight(0.0)
    {
    }
};

TEST_CLASS(Graph_Test)
{
    QByteArray string_db_file{"data_in/string_db_interactions.txt"};
    QByteArray alias_file{"data_in/string_db_aliases.txt"};

private:
    TEST_METHOD(testUndirectedGraph)
    {
        // create undirected graph with NodeType and EdgeType int
        Graph<int, int> graph;
        IS_FALSE(graph.directed());

        // add some nodes
        for(int i = 1; i <= 20; i++)
        {
            IS_TRUE(graph.addNode(QString::number(i), i%4))
        }

        // add some edges (regular pattern)
        for(int i = 1; i <= 20; i++)
        {
            if(i < 20)
            {
                IS_TRUE(graph.addEdge(graph.getNode(QString::number(i)),
                                      graph.getNode(QString::number(i+1)),
                                      1));
                if(i < 19 && i%2 == 1)
                {
                    IS_TRUE(graph.addEdge(graph.getNode(QString::number(i)),
                                          graph.getNode(QString::number(i+2)),
                                          3));
                }
                else if(i == 19)
                {
                    IS_TRUE(graph.addEdge(graph.getNode("19"),
                                          graph.getNode("1"),
                                          3))
                }
            }
            else if(i == 20)
            {
                IS_TRUE(graph.addEdge(graph.getNode("20"),
                                      graph.getNode("1"),
                                      1))
            }
        }

        // test behavior when trying to add nodes or edges that already exist
        IS_THROWN(ArgumentException, graph.addNode("1", 2));
        IS_FALSE(graph.addNode("1", 2, false));
        IS_FALSE(graph.addEdge("3", 3, "4", 0, 1));
        IS_FALSE(graph.addEdge(graph.getNode("11"), graph.getNode("13"), 3));

        // test behavior when adding additional edges with method that also creates nodes if necessary
        // between one existing and one new node
        IS_TRUE(graph.addEdge("2", 2, "21", 1, 2));
        // between two new nodes
        IS_TRUE(graph.addEdge("22", 2, "23", 3, 1));
        // between existing nodes
        IS_TRUE(graph.addEdge("1", 1, "11", 3, 10));
        IS_TRUE(graph.addEdge("20", 0, "22", 2, 1));

        // test behavior when checking if nodes exist / accessing nodes
        IS_TRUE(graph.hasNode("4"));
        IS_TRUE(graph.hasNode("23"));
        IS_FALSE(graph.hasNode("24"));
        S_EQUAL(graph.getNode("16").data()->nodeName(), "16");
        IS_THROWN(ArgumentException, graph.getNode("99"));

        // test behavior when checking if edges exist / accessing edges
        IS_TRUE(graph.hasEdge("15", "17"));
        IS_TRUE(graph.hasEdge("17", "15"));
        IS_FALSE(graph.hasEdge("14", "16"));
        IS_FALSE(graph.hasEdge("16", "14"));
        IS_FALSE(graph.hasEdge("20", "24"));
        X_EQUAL(graph.getEdge("1", "11"), graph.getEdge("11", "1"));
        IS_THROWN(ArgumentException, graph.getEdge("99", "20"));
        IS_THROWN(ArgumentException, graph.getEdge("2", "4"));

        // check content of some nodes/edges
        I_EQUAL(graph.getNode("5").data()->nodeContent(), 1);
        I_EQUAL(graph.getNode("16").data()->nodeContent(), 0);
        I_EQUAL(graph.getEdge("1", "11").data()->edgeContent(), 10);
        I_EQUAL(graph.getEdge("11", "9").data()->edgeContent(), 3);

        // check connectivity: adjacent nodes
        IS_TRUE(graph.isAdjacent("1", "11"));
        IS_TRUE(graph.isAdjacent("11", "1"));
        IS_TRUE(graph.isAdjacent("5", "7"));
        IS_TRUE(graph.isAdjacent("7", "5"));
        IS_FALSE(graph.isAdjacent("8", "10"));
        IS_THROWN(ArgumentException, graph.isAdjacent("99", "20"));

        // check connectivity: node degrees
        for(int i = 1; i <= 23; i++)
        {
            if(i%2 == 0)
            {
                switch(i)
                {
                    case 2:
                    case 20:
                        I_EQUAL(graph.getDegree(QString::number(i)), 3);
                        break;
                    default:
                        I_EQUAL(graph.getDegree(QString::number(i)), 2);
                }
            }
            else
            {
                switch(i)
                {
                    case 1:
                    case 11:
                        I_EQUAL(graph.getDegree(QString::number(i)), 5);
                        break;
                    case 21:
                    case 23:
                        I_EQUAL(graph.getDegree(QString::number(i)), 1);
                        break;
                    default:
                        I_EQUAL(graph.getDegree(QString::number(i)), 4);
                }            }
        }

        // test behavior when trying to use methods that are only available for directed graphs
        IS_THROWN(Exception, graph.getIndegree("1"));
        IS_THROWN(Exception, graph.getOutdegree("1"));
    }

    TEST_METHOD(testDirectedGraph)
    {
        // create directed graph with NodeType and EdgeType int
        Graph<int, int> graph(true);
        IS_TRUE(graph.directed());

        // add some nodes
        for(int i = 1; i <= 20; i++)
        {
            IS_TRUE(graph.addNode(QString::number(i), i%4))
        }

        // add some edges (regular pattern)
        for(int i = 1; i <= 20; i++)
        {
            if(i < 20)
            {
                IS_TRUE(graph.addEdge(graph.getNode(QString::number(i)),
                                      graph.getNode(QString::number(i+1)),
                                      1));
                if(i < 19 && i%2 == 1)
                {
                    IS_TRUE(graph.addEdge(graph.getNode(QString::number(i)),
                                          graph.getNode(QString::number(i+2)),
                                          3));
                }
                else if(i == 19)
                {
                    IS_TRUE(graph.addEdge(graph.getNode("19"),
                                          graph.getNode("1"),
                                          3))
                }
            }
            else if(i == 20)
            {
                IS_TRUE(graph.addEdge(graph.getNode("20"),
                                      graph.getNode("1"),
                                      1))
            }
        }

        // test behavior when trying to add nodes or edges that already exist
        IS_THROWN(ArgumentException, graph.addNode("1", 2));
        IS_FALSE(graph.addNode("1", 2, false));
        IS_FALSE(graph.addEdge("3", 3, "4", 0, 1));
        IS_FALSE(graph.addEdge(graph.getNode("11"), graph.getNode("13"), 3));

        // test behavior when adding additional edges with method that also creates nodes if necessary
        // between one existing and one new node
        IS_TRUE(graph.addEdge("2", 2, "21", 1, 2));
        // between two new nodes
        IS_TRUE(graph.addEdge("22", 2, "23", 3, 1));
        // between existing nodes
        IS_TRUE(graph.addEdge("1", 1, "11", 3, 10));
        IS_TRUE(graph.addEdge("20", 0, "22", 2, 1));

        // test behavior when checking if nodes exist / accessing nodes
        IS_TRUE(graph.hasNode("4"));
        IS_TRUE(graph.hasNode("23"));
        IS_FALSE(graph.hasNode("24"));
        S_EQUAL(graph.getNode("16").data()->nodeName(), "16");
        IS_THROWN(ArgumentException, graph.getNode("99"));

        // test behavior when checking if edges exist / accessing edges
        IS_TRUE(graph.hasEdge("15", "17"));
        IS_FALSE(graph.hasEdge("17", "15"));
        IS_FALSE(graph.hasEdge("14", "16"));
        IS_FALSE(graph.hasEdge("16", "14"));
        IS_FALSE(graph.hasEdge("20", "24"));
        IS_THROWN(ArgumentException, graph.getEdge("99", "20"));
        IS_THROWN(ArgumentException, graph.getEdge("2", "4"));
        IS_THROWN(ArgumentException, graph.getEdge("2", "1"));

        // check content of some nodes/edges
        I_EQUAL(graph.getNode("5").data()->nodeContent(), 1);
        I_EQUAL(graph.getNode("16").data()->nodeContent(), 0);
        I_EQUAL(graph.getEdge("1", "11").data()->edgeContent(), 10);
        I_EQUAL(graph.getEdge("9", "11").data()->edgeContent(), 3);

        // check connectivity: adjacent nodes
        IS_TRUE(graph.isAdjacent("1", "11"));
        IS_FALSE(graph.isAdjacent("11", "1"));
        IS_TRUE(graph.isAdjacent("5", "7"));
        IS_FALSE(graph.isAdjacent("7", "5"));
        IS_FALSE(graph.isAdjacent("8", "10"));
        IS_THROWN(ArgumentException, graph.isAdjacent("99", "20"));

        // check connectivity: node in- and outdegrees
        for(int i = 1; i <= 23; i++)
        {
            if(i%2 == 0)
            {
                I_EQUAL(graph.getIndegree(QString::number(i)), 1);
                switch(i)
                {
                    case 2:
                    case 20:
                        I_EQUAL(graph.getOutdegree(QString::number(i)), 2);
                        break;
                    default:
                        I_EQUAL(graph.getOutdegree(QString::number(i)), 1);
                }
            }
            else
            {
                switch(i)
                {
                    case 1:
                        I_EQUAL(graph.getIndegree(QString::number(i)), 2);
                        I_EQUAL(graph.getOutdegree(QString::number(i)), 3);
                        break;
                    case 11:
                        I_EQUAL(graph.getIndegree(QString::number(i)), 3);
                        I_EQUAL(graph.getOutdegree(QString::number(i)), 2);
                        break;
                    case 21:
                    case 23:
                        I_EQUAL(graph.getIndegree(QString::number(i)), 1);
                        I_EQUAL(graph.getOutdegree(QString::number(i)), 0);
                        break;
                    default:
                        I_EQUAL(graph.getIndegree(QString::number(i)), 2);
                        I_EQUAL(graph.getOutdegree(QString::number(i)), 2);
                }
            }
        }

        // test behavior when trying to use method that is only available for undirected graphs
        IS_THROWN(Exception, graph.getDegree("1"));
    }
};
