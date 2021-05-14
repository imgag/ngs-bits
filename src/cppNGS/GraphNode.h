#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include "cppNGS_global.h"
#include <QList>
#include <QSharedPointer>

template <typename NodeType>
class CPPNGSSHARED_EXPORT GraphNode
{
    public:
        // constructors
        GraphNode();
        GraphNode(const NodeType& content);

        // getters
        const NodeType& nodeContent() const;
        NodeType& nodeContent();

        // comparison with another node
        bool isEqual(QSharedPointer<GraphNode<NodeType>> other);

    private:
        NodeType node_content;
};

#endif // GRAPHNODE_H
