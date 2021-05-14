#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H

#include "cppNGS_global.h"
#include "GraphNode.h"
#include <QSharedPointer>

template <typename EdgeType, typename NodeType>
class GraphEdge
{
    public:
        // constructors
        GraphEdge();
        GraphEdge(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2, const EdgeType& content);

        // getters
        QSharedPointer<const GraphNode<NodeType>> node1() const;
        QSharedPointer<GraphNode<NodeType>> node1();
        QSharedPointer<const GraphNode<NodeType>> node2() const;
        QSharedPointer<GraphNode<NodeType>> node2();

        const EdgeType& edgeContent() const;
        EdgeType& edgeContent();

        // setters
        void setNodes(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2);

    private:
        // the two nodes connected by this edge
        QSharedPointer<GraphNode<NodeType>> node_1_;
        QSharedPointer<GraphNode<NodeType>> node_2_;

        EdgeType edge_content_;
};

template <typename EdgeType, typename NodeType>
GraphEdge<EdgeType, NodeType>::GraphEdge()
    : edge_content_()
{
}

template <typename EdgeType, typename NodeType>
GraphEdge<EdgeType, NodeType>::GraphEdge(QSharedPointer<GraphNode<NodeType> > node_1, QSharedPointer<GraphNode<NodeType> > node_2, const EdgeType &content)
    : node_1_(node_1),
      node_2_(node_2),
      edge_content_(content)
{
}

template <typename EdgeType, typename NodeType>
QSharedPointer<const GraphNode<NodeType>> GraphEdge<EdgeType, NodeType>::node1() const
{
    return node_1_;
}

template <typename EdgeType, typename NodeType>
QSharedPointer<GraphNode<NodeType>> GraphEdge<EdgeType, NodeType>::node1()
{
    return node_1_;
}

template <typename EdgeType, typename NodeType>
QSharedPointer<const GraphNode<NodeType>> GraphEdge<EdgeType, NodeType>::node2() const
{
    return node_1_;
}

template <typename EdgeType, typename NodeType>
QSharedPointer<GraphNode<NodeType>> GraphEdge<EdgeType, NodeType>::node2()
{
    return node_1_;
}

template <typename EdgeType, typename NodeType>
const EdgeType& GraphEdge<EdgeType, NodeType>::edgeContent() const
{
    return edge_content_;
}

template <typename EdgeType, typename NodeType>
EdgeType& GraphEdge<EdgeType, NodeType>::edgeContent()
{
    return edge_content_;
}

template <typename EdgeType, typename NodeType>
void GraphEdge<EdgeType, NodeType>::setNodes(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2)
{
    node_1_ = node_1;
    node_2_ = node_2;
}


#endif // GRAPHEDGE_H
