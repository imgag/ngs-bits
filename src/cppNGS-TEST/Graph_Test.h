#include "TestFramework.h"
#include "Graph.h"

TEST_CLASS(Graph_Test)
{
Q_OBJECT
private slots:
    void createIntGraph()
    {
        Graph<int, int> graph;
        graph.addNode(15, "first");
        graph.addNode(2, "second");
        graph.getNode("first");
        graph.addEdge(5, "third", 2, "fourth", 4);
        graph.addEdge(5, "third", 2, "fourth", 4);
        graph.getEdge("third", "fourth");
        graph.addEdge(graph.getNode("first"), graph.getNode("third"), 10);
    }
};
