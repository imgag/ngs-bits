#ifndef GRAPH_H
#define GRAPH_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include <QList>
#include <QSharedPointer>
#include "GraphNode.h"
#include "GraphEdge.h"

template <typename NodeType, typename EdgeType>
class CPPNGSSHARED_EXPORT Graph
{
    public:
        using NodePointer = QSharedPointer<GraphNode<NodeType>>;
        using EdgePointer = QSharedPointer<GraphEdge<EdgeType, NodeType>>;

        Graph();
        Graph(bool directed);

        // getters
        const QList<NodePointer>& nodeList() const;
        const QList<EdgePointer>& edgeList() const;
        const QHash<NodePointer, QList<EdgePointer>>& adjacencyList() const;
        bool directed();

        // create and add node
        bool addNode(const NodeType& content, const QString& name, bool throw_exception_if_contained = true);

        // add edge between existing nodes
        bool addEdge(NodePointer node_1, NodePointer node_2,
                     const EdgeType& content);
        // create nodes and add an edge between them
        bool addEdge(const NodeType& node_content_1, const QString& name_1,
                     const NodeType& node_content_2, const QString& name_2,
                     const EdgeType& edge_content);

        // check if specific node exists in the graph
        bool hasNode(const QString& name);
        bool hasNode(NodePointer node);

        // get specific node
        NodePointer getNode(const QString& name);

        // check if specific edge/edge between specific nodes exists
        bool hasEdge(EdgePointer edge);
        bool hasEdge(NodePointer node_1, NodePointer node_2);
        bool hasEdge(const QString& node_name_1, const QString& node_name_2);

        // get edge between specific nodes
        EdgePointer getEdge(NodePointer node_1, NodePointer node_2);
        EdgePointer getEdge(const QString& node_name_1, const QString& node_name_2);

    protected:
        // add existing edge
        bool addEdge(EdgePointer edge);
        // add existing node
        bool addNode(NodePointer node, bool throw_exception_if_contained = true);

    private:
        QList<NodePointer> node_list_;
        QList<EdgePointer> edge_list_;

        // store an adjacency list for each node
        QHash<NodePointer, QList<EdgePointer>> adjacency_list_;

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
const QList<typename Graph<NodeType, EdgeType>::NodePointer>& Graph<NodeType, EdgeType>::nodeList() const
{
    return node_list_;
}

template <typename NodeType, typename EdgeType>
const QList<typename Graph<NodeType, EdgeType>::EdgePointer>& Graph<NodeType, EdgeType>::edgeList() const
{
    return edge_list_;
}

template <typename NodeType, typename EdgeType>
const QHash<typename Graph<NodeType, EdgeType>::NodePointer, QList<typename Graph<NodeType, EdgeType>::EdgePointer>>& Graph<NodeType, EdgeType>::adjacencyList() const
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
bool Graph<NodeType, EdgeType>::addNode(NodePointer node, bool throw_exception_if_contained)
{
    if(!node_list_.contains(node))
    {
        node_list_.append(node);
        // add the node to the adjacency list with an empty list of edges
        adjacency_list_.insert(node, QList<EdgePointer>());
        return true;
    }
    else if(throw_exception_if_contained)
    {
        THROW(ArgumentException, "Invalid argument: Node already in graph")
    }

    return false;
}

// generate a new node and add it to the graph
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::addNode(const NodeType& content, const QString& name,
                                        bool throw_exception_if_contained)
{
    if(name.isEmpty())
    {
        THROW(ArgumentException, "Invalid argument: Empty node name")
    }

    if(!this->hasNode(name))
    {
        NodePointer node(new GraphNode<NodeType>(content, name));
        this->addNode(node);
        return true;
    }
    else if (throw_exception_if_contained)
    {
        THROW(ArgumentException, "Invalid argument: Node of specified name already in graph");
    }

    return false;
}


// check by name if a specific node exists in the graph
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasNode(const QString &name)
{
    foreach(const NodePointer& node, node_list_)
    {
        if(node.data()->nodeName() == name)
        {
            return true;
        }
    }
    return false;
}

// check if node of the same name as given node exists in the graph
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasNode(NodePointer node)
{
    return hasNode(node.data()->nodeName());
}

// get a specific node by name
template <typename NodeType, typename EdgeType>
typename Graph<NodeType, EdgeType>::NodePointer Graph<NodeType, EdgeType>::getNode(const QString &name)
{
    foreach(const NodePointer& node, node_list_)
    {
        if(node.data()->nodeName() == name)
        {
            return node;
        }
    }
    THROW(ArgumentException, "Invalid argument: Non-existing node")
}


// adding edges to the graph

// add an edge to the graph (only used internally; protected)
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::addEdge(EdgePointer edge)
{
    edge_list_.append(edge);
    adjacency_list_[edge.data()->node1()].append(edge);
    if(!directed_)
    {
        adjacency_list_[edge.data()->node2()].append(edge);
    }
    return true;
}

// add an edge between two nodes; throw exception if nodes do not exist
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::addEdge(NodePointer node_1,
                                        NodePointer node_2,
                                        const EdgeType& content)
{
    if(!hasNode(node_1) || !hasNode(node_2))
    {
        THROW(ArgumentException, "Invalid argument: Non-existing node")
    }

    if(!hasEdge(node_1, node_2))
    {
        EdgePointer edge(new GraphEdge<EdgeType, NodeType>(node_1, node_2, content));
        return addEdge(edge);
    }
    return false;
}

// generate two nodes with content and name and add an edge between them
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::addEdge(const NodeType& node_content_1, const QString& name_1,
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
        EdgePointer edge(new GraphEdge<EdgeType, NodeType>(node_1, node_2, edge_content));
        edge_list_.append(edge);
        adjacency_list_[node_1].append(edge);
        if(!directed_)
        {
            adjacency_list_[node_2].append(edge);
        }
        return true;
    }
    return false;
}

// check if a specific edge exists
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasEdge(EdgePointer edge)
{
    return hasEdge(edge.data()->node1(), edge.data()->node2());
}

// check if a specific edge exists by the nodes it connects
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasEdge(NodePointer node_1, NodePointer node_2)
{
    EdgePointer edge;
    foreach(edge, edge_list_)
    {
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
bool Graph<NodeType, EdgeType>::hasEdge(const QString& node_name_1, const QString& node_name_2)
{
    EdgePointer edge;
    foreach(edge, edge_list_)
    {
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
typename Graph<NodeType, EdgeType>::EdgePointer Graph<NodeType, EdgeType>::getEdge(NodePointer node_1, NodePointer node_2)
{
    EdgePointer edge;
    foreach(edge, edge_list_)
    {
        if(edge.data()->node1() == node_1 && edge.data()->node2() == node_2) {
            return edge;
        }
        else if(!directed_ && edge.data()->node1() == node_2 && edge.data()->node2() == node_1)
        {
            return edge;
        }
    }
    THROW(ArgumentException, "Invalid argument: Non-existing edge")
}

// get an edge from the names of the two nodes it connects
template <typename NodeType, typename EdgeType>
typename Graph<NodeType, EdgeType>::EdgePointer Graph<NodeType, EdgeType>::getEdge(const QString &node_name_1, const QString &node_name_2)
{
    EdgePointer edge;
    foreach(edge, edge_list_)
    {
        if(edge.data()->node1().data()->nodeName() == node_name_1 &&
                edge.data()->node2().data()->nodeName() == node_name_2) {
            return edge;
        }
        else if(!directed_ && edge.data()->node1()->nodeName() == node_name_2 &&
                edge.data()->node2().data()->nodeName() == node_name_1)
        {
            return edge;
        }
    }
    THROW(ArgumentException, "Invalid argument: Non-existing edge")
}


#endif // GRAPH_H
