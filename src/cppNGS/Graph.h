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
        Graph(bool directed);

        // getters
        const QList<GraphNode<NodeType>>& nodeList() const;
        const QList<GraphEdge<EdgeType, NodeType>>& edgeList() const;
        const QHash<QSharedPointer<GraphNode<NodeType>>, QList<QSharedPointer<GraphEdge<EdgeType, NodeType>>>>& adjacencyList() const;
        bool directed();

        // add existing node
        void addNode(QSharedPointer<GraphNode<NodeType>> node);
        // create and add node
        void addNode(const NodeType& content, const QString& name);

        // add existing edge
        void addEdge(QSharedPointer<GraphEdge<EdgeType, NodeType>> edge);
        // add edge between existing nodes
        void addEdge(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2,
                     const EdgeType& content);
        // create nodes and add an edge between them
        void addEdge(const NodeType& node_content_1, const QString& name_1,
                     const NodeType& node_content_2, const QString& name_2,
                     const EdgeType& edge_content);

        // check if specific node exists in the graph
        bool hasNode(const QString& name);
        bool hasNode(QSharedPointer<GraphNode<NodeType>> node);

        // get specific node
        QSharedPointer<GraphNode<NodeType>> getNode(const QString& name);

        // check if specific edge/edge between specific nodes exists
        bool hasEdge(QSharedPointer<GraphEdge<EdgeType, NodeType>> edge);
        bool hasEdge(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2);
        bool hasEdge(const QString& node_name_1, const QString& node_name_2);

        // get edge between specific nodes
        QSharedPointer<GraphEdge<EdgeType, NodeType>> getEdge(QSharedPointer<GraphNode<NodeType>> node_1,
                                                             QSharedPointer<GraphNode<NodeType>> node_2);
        QSharedPointer<GraphEdge<EdgeType, NodeType>> getEdge(const QString& node_name_1, const QString& node_name_2);

    private:
        QList<QSharedPointer<GraphNode<NodeType>>> node_list_;
        QList<QSharedPointer<GraphEdge<EdgeType, NodeType>>> edge_list_;

        // store an adjacency list for each node
        QHash<QSharedPointer<GraphNode<NodeType>>, QList<QSharedPointer<GraphEdge<EdgeType, NodeType>>>> adjacency_list_;

        bool directed_;
};

// default constuctor: undirected graph
template <typename NodeType, typename EdgeType>
Graph<NodeType, EdgeType>::Graph()
    : node_list_(),
      edge_list_(),
      adjacency_list_(),
      directed_(false)
{
}

// constructor that allows making a directed graph
template <typename NodeType, typename EdgeType>
Graph<NodeType, EdgeType>::Graph(bool directed)
    : node_list_(),
      edge_list_(),
      adjacency_list_(),
      directed_(directed)
{
}


// getters

template <typename NodeType, typename EdgeType>
const QList<GraphNode<NodeType>>& Graph<NodeType, EdgeType>::nodeList() const
{
    return node_list_;
}

template <typename NodeType, typename EdgeType>
const QList<GraphEdge<EdgeType, NodeType>>& Graph<NodeType, EdgeType>::edgeList() const
{
    return edge_list_;
}

template <typename NodeType, typename EdgeType>
const QHash<QSharedPointer<GraphNode<NodeType>>, QList<QSharedPointer<GraphEdge<EdgeType, NodeType>>>>& Graph<NodeType, EdgeType>::adjacencyList() const
{
    return adjacency_list_;
}

template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::directed()
{
    return directed_;
}


// adding nodes to the graph

// add an existing node to the graph
template <typename NodeType, typename EdgeType>
void Graph<NodeType, EdgeType>::addNode(QSharedPointer<GraphNode<NodeType> > node)
{
    if(!node_list_.contains(node))
    {
        node_list_.append(node);
        // add the node to the adjacency list with an empty list of edges
        adjacency_list_.insert(node, QList<QSharedPointer<GraphEdge<EdgeType, NodeType>>>());
    }
}

// generate a new node and add it to the graph
template <typename NodeType, typename EdgeType>
void Graph<NodeType, EdgeType>::addNode(const NodeType& content, const QString& name)
{
    if(!this->hasNode(name))
    {
        QSharedPointer<GraphNode<NodeType>> node(new GraphNode<NodeType>(content, name));
        this->addNode(node);
    }
}


// check by name if a specific node exists in the graph
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasNode(const QString &name)
{
    QListIterator<QSharedPointer<GraphNode<NodeType>>> iterator(node_list_);
    while(iterator.hasNext())
    {
        if(iterator.next().data()->nodeName() == name) {
            return true;
        }
    }
    return false;
}

// check if node of the same name as given node exists in the graph
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasNode(QSharedPointer<GraphNode<NodeType>> node)
{
    return hasNode(node.data()->nodeName());
}

// get a specific node by name
template <typename NodeType, typename EdgeType>
QSharedPointer<GraphNode<NodeType>> Graph<NodeType, EdgeType>::getNode(const QString &name)
{
    QListIterator<QSharedPointer<GraphNode<NodeType>>> iterator(node_list_);
    while(iterator.hasNext())
    {
        auto node = iterator.next();
        if(node.data()->nodeName() == name) {
            return node;
        }
    }
    return QSharedPointer<GraphNode<NodeType>>();
}


// adding edges to the graph

// add an existing edge to the graph (only if the nodes it connects already exist)
template <typename NodeType, typename EdgeType>
void Graph<NodeType, EdgeType>::addEdge(QSharedPointer<GraphEdge<EdgeType, NodeType>> edge)
{
    if(!hasEdge(edge) && hasNode(edge.data()->node1()) && hasNode(edge.data()->node2()))
    {
        edge_list_.append(edge);
        adjacency_list_[edge.data()->node1()].append(edge);
        if(!directed_)
        {
            adjacency_list_[edge.data()->node2()].append(edge);
        }
    }
}

// add an edge between two nodes; generate nodes if necessary
template <typename NodeType, typename EdgeType>
void Graph<NodeType, EdgeType>::addEdge(QSharedPointer<GraphNode<NodeType>> node_1,
                                        QSharedPointer<GraphNode<NodeType>> node_2,
                                        const EdgeType& content)
{
    // if nodes not in graph: add them
    if(!hasNode(node_1))
    {
        addNode(node_1);
    }
    if(!hasNode(node_2))
    {
        addNode(node_2);
    }

    if(!hasEdge(node_1, node_2))
    {
        QSharedPointer<GraphEdge<EdgeType, NodeType>> edge(new GraphEdge<EdgeType, NodeType>(node_1, node_2, content));
        edge_list_.append(edge);
        adjacency_list_[node_1].append(edge);
        if(!directed_)
        {
            adjacency_list_[node_2].append(edge);
        }
    }
}

// generate two nodes with content and name and add an edge between them
template <typename NodeType, typename EdgeType>
void Graph<NodeType, EdgeType>::addEdge(const NodeType& node_content_1, const QString& name_1,
                                        const NodeType& node_content_2, const QString& name_2,
                                        const EdgeType& edge_content)
{
    // only create and add nodes if they do not yet exist
    if(!hasNode(name_1))
    {
        addNode(node_content_1, name_1);
    }
    if(!hasNode(name_2))
    {
        addNode(node_content_2, name_2);
    }

    if(!hasEdge(name_1, name_2))
    {
        auto node_1 = getNode(name_1);
        auto node_2 = getNode(name_2);
        QSharedPointer<GraphEdge<EdgeType, NodeType>> edge(new GraphEdge<EdgeType, NodeType>(node_1, node_2, edge_content));
        edge_list_.append(edge);
        adjacency_list_[node_1].append(edge);
        if(!directed_)
        {
            adjacency_list_[node_2].append(edge);
        }
    }
}

// check if a specific edge exists
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasEdge(QSharedPointer<GraphEdge<EdgeType, NodeType>> edge)
{
    return hasEdge(edge.data()->node1(), edge.data()->node2());
}

// check if a specific edge exists by the nodes it connects
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasEdge(QSharedPointer<GraphNode<NodeType>> node_1, QSharedPointer<GraphNode<NodeType>> node_2)
{
    QListIterator<QSharedPointer<GraphEdge<EdgeType, NodeType>>> iterator(edge_list_);
    while(iterator.hasNext())
    {
        auto edge = iterator.next();
        if(edge.data()->node1() == node_1 && edge.data()->node2() == node_2) {
            return true;
        }
        else if(!directed_ && edge.data()->node1() == node_2 && edge.data()->node2() == node_1)
        {
            return true;
        }
    }
    return false;
}

// check if a specific edge exists by the names of the nodes that it connects
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasEdge(const QString &node_name_1, const QString &node_name_2)
{
    QListIterator<QSharedPointer<GraphEdge<EdgeType, NodeType>>> iterator(edge_list_);
    while(iterator.hasNext())
    {
        auto edge = iterator.next();
        if(edge.data()->node1().data()->nodeName() == node_name_1 &&
                edge.data()->node2().data()->nodeName() == node_name_2) {
            return true;
        }
        else if(!directed_ && edge.data()->node1().data()->nodeName() == node_name_2 &&
                edge.data()->node2().data()->nodeName() == node_name_1)
        {
            return true;
        }
    }
    return false;
}

// get an edge from the two nodes that it connects
template <typename NodeType, typename EdgeType>
QSharedPointer<GraphEdge<EdgeType, NodeType>> Graph<NodeType, EdgeType>::getEdge(QSharedPointer<GraphNode<NodeType> > node_1, QSharedPointer<GraphNode<NodeType> > node_2)
{
    QListIterator<QSharedPointer<GraphEdge<EdgeType, NodeType>>> iterator(edge_list_);
    while(iterator.hasNext())
    {
        auto edge = iterator.next();
        if(edge.data()->node1() == node_1 &&
                edge.data()->node2() == node_2) {
            return edge;
        }
        else if(!directed_ && edge.data()->node1() == node_2 &&
                edge.data()->node2() == node_1)
        {
            return edge;
        }
    }
    return QSharedPointer<GraphEdge<EdgeType, NodeType>>();
}

// get an edge from the names of the two nodes it connects
template <typename NodeType, typename EdgeType>
QSharedPointer<GraphEdge<EdgeType, NodeType>> Graph<NodeType, EdgeType>::getEdge(const QString &node_name_1, const QString &node_name_2)
{
    QListIterator<QSharedPointer<GraphEdge<EdgeType, NodeType>>> iterator(edge_list_);
    while(iterator.hasNext())
    {
        auto edge = iterator.next();
        if(edge.data()->node1().data()->nodeName() == node_name_1 &&
                edge.data()->node2().data()->nodeName() == node_name_2) {
            return edge;
        }
        else if(!directed_ && edge.data()->node1().data()->nodeName() == node_name_2 &&
                edge.data()->node2().data()->nodeName() == node_name_1)
        {
            return edge;
        }
    }
    return QSharedPointer<GraphEdge<EdgeType, NodeType>>();
}


#endif // GRAPH_H
