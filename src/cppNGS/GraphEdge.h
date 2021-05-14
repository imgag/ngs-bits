#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H

#include "cppNGS_global.h"
#include "GraphNode.h"
#include <QSharedPointer>

template <typename EdgeType, typename NodeType>
class CPPNGSSHARED_EXPORT GraphEdge
{
    public:
        // constructors
        GraphEdge();
        GraphEdge(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2, const EdgeType& content);

        // getters
        QSharedPointer<GraphNode<NodeType>> node1();
        QSharedPointer<GraphNode<NodeType>> node2();
        const EdgeType& edgeContent() const;
        EdgeType& edgeContent();

        // setters
        void setEdgeWeight();
        void setNodes(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2);

        // comparison with another edge
        bool isEqual(QSharedPointer<GraphEdge<EdgeType, NodeType>>);

    private:
        // the two nodes connected by this edge
        QSharedPointer<GraphNode<NodeType>> node_1;
        QSharedPointer<GraphNode<NodeType>> node_2;

        EdgeType edge_content;
};

#endif // GRAPHEDGE_H
