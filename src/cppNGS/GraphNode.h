#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include "cppNGS_global.h"
#include <QString>

template <typename NodeType>
class GraphNode
{
    public:
        // constructors
        GraphNode();
        GraphNode(const NodeType& content, const QString& name);

        // getters
        const NodeType& nodeContent() const;
        NodeType& nodeContent();
        const QString& nodeName() const;

    private:
        NodeType node_content_;
        QString node_name_;
};

template <typename NodeType>
GraphNode<NodeType>::GraphNode()
{
}

template <typename NodeType>
GraphNode<NodeType>::GraphNode(const NodeType &content, const QString& name)
    : node_content_(content),
      node_name_(name)
{
}

template <typename NodeType>
const NodeType& GraphNode<NodeType>::nodeContent() const
{
    return node_content_;
}

template <typename NodeType>
NodeType& GraphNode<NodeType>::nodeContent()
{
    return node_content_;
}

template <typename NodeType>
const QString& GraphNode<NodeType>::nodeName() const
{
    return node_name_;
}

#endif // GRAPHNODE_H
