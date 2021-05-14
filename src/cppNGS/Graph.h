#ifndef GRAPH_H
#define GRAPH_H

#include "cppNGS_global.h"
#include <QList>
#include <QSharedPointer>
#include "GraphNode.h"
#include "GraphEdge.h"

template <typename NodeType, typename EdgeType>
class CPPNGSSHARED_EXPORT Graph
{
    public:
        Graph();
        // add existing node
        void addNode(QSharedPointer<GraphNode<NodeType>> node);
        // create and add node
        void addNode(const NodeType& content);
        // add existing edge
        void addEdge(QSharedPointer<GraphEdge<EdgeType, NodeType>>);
        // add edge between existing nodes
        void addEdge(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2, const EdgeType& content);
        // create nodes and add an edge between them
        void addEdge(const NodeType& node_content_1, const NodeType& node_content_2, const EdgeType& edge_content);

        bool hasNode(const NodeType& node_content);
        bool hasEdge(QSharedPointer<GraphNode<NodeType>>, QSharedPointer<GraphNode<NodeType>>);

        QList<GraphNode<NodeType>> nodesList();
        QList<GraphEdge<EdgeType, NodeType>> edgeList();
        QHash<QSharedPointer<GraphNode<NodeType>>, QList<QSharedPointer<GraphEdge<EdgeType, NodeType>>>> adjacencyList();

    private:
        QList<QSharedPointer<GraphNode<NodeType>>> nodes_list;
        QList<QSharedPointer<GraphEdge<EdgeType, NodeType>>> edge_list;
        QHash<QSharedPointer<GraphNode<NodeType>>, QList<QSharedPointer<GraphEdge<EdgeType, NodeType>>>> adjacency_list;
};

#endif // GRAPH_H
