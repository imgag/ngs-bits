#ifndef GRAPH_H
#define GRAPH_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QList>
#include <QHash>
#include <QSet>
#include <QPair>
#include <QSharedPointer>
#include <QFile>
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
        const QHash<QString, NodePointer>& nodeList() const;
        const QHash<NodePointer, QList<EdgePointer>>& adjacencyList() const;
        bool directed();

        // create and add node
        bool addNode(const QString& name, const NodeType& content, bool throw_exception_if_contained = true);

        // add edge between existing nodes
        bool addEdge(NodePointer node_1, NodePointer node_2,
                     const EdgeType& content);
        // create nodes and add an edge between them
        bool addEdge(const QString& name_1, const NodeType& node_content_1,
                     const QString& name_2, const NodeType& node_content_2,
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

        // get adjacent edges/nodes for a particular node
        QList<NodePointer> getAdjacentNodes(const QString& name);
        QList<EdgePointer> getAdjacentEdges(const QString& name);

        // check if node with name_2 is adjacent to node with name_1
        bool isAdjacent(const QString& name_1, const QString& name_2);

        // get degree of specific node
        int getDegree(const QString& name);

        // only for directed graphs
        int getIndegree(const QString& name);
        int getOutdegree(const QString& name);

        void store(const QString& file);

    protected:
        // add existing edge
        bool addEdge(EdgePointer edge);
        // add existing node
        bool addNode(NodePointer node, bool throw_exception_if_contained = true);

    private:
        QHash<QString, NodePointer> node_list_;

        // only for directed graph
        QHash<QString, int> indegree_;

        // store an adjacency list for each node
        QHash<NodePointer, QList<EdgePointer>> adjacency_list_;

        QSet<QPair<QString, QString>> edge_list_;

        bool directed_;
};

// default constuctor: undirected graph
template <typename NodeType, typename EdgeType>
Graph<NodeType, EdgeType>::Graph()
    : node_list_(),
      adjacency_list_(),
      edge_list_(),
      directed_(false)
{
}

// constructor that allows making a directed graph
template <typename NodeType, typename EdgeType>
Graph<NodeType, EdgeType>::Graph(bool directed)
    : node_list_(),
      adjacency_list_(),
      edge_list_(),
      directed_(directed)
{
}


// getters

template <typename NodeType, typename EdgeType>
const QHash<QString, typename Graph<NodeType, EdgeType>::NodePointer>& Graph<NodeType, EdgeType>::nodeList() const
{
    return node_list_;
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
    if(!hasNode(node))
    {
        node_list_.insert(node.data()->nodeName(), node);
        // add the node to the adjacency list with an empty list of edges
        adjacency_list_.insert(node, QList<EdgePointer>());
        if(directed_)
        {
            indegree_.insert(node.data()->nodeName(), 0);
        }
        return true;
    }
    else if(throw_exception_if_contained)
    {
        THROW(ArgumentException, "Invalid argument: Node already in graph");
    }

    return false;
}

// generate a new node and add it to the graph
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::addNode(const QString& name, const NodeType& content,
                                        bool throw_exception_if_contained)
{
    if(name.isEmpty())
    {
        THROW(ArgumentException, "Invalid argument: Empty node name");
    }

	if(!hasNode(name))
    {
        NodePointer node(new GraphNode<NodeType>(content, name));
		return addNode(node);
    }
    else if (throw_exception_if_contained)
    {
        THROW(ArgumentException, "Invalid argument: Node of specified name already in graph");
    }

    return false;
}


// check by name if a specific node exists in the graph
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasNode(const QString& name)
{
    return node_list_.contains(name);
}

// check if node of the same name as given node exists in the graph
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasNode(NodePointer node)
{
    return hasNode(node.data()->nodeName());
}

// get a specific node by name
template <typename NodeType, typename EdgeType>
typename Graph<NodeType, EdgeType>::NodePointer Graph<NodeType, EdgeType>::getNode(const QString& name)
{
    if(hasNode(name))
    {
        return node_list_.value(name);
    }
    THROW(ArgumentException, "Invalid argument: Non-existing node");
}


// adding edges to the graph

// add an edge to the graph (only used internally; protected)
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::addEdge(EdgePointer edge)
{
    adjacency_list_[edge.data()->node1()].append(edge);
    edge_list_.insert(QPair<QString, QString>(edge.data()->node1().data()->nodeName(),
                      edge.data()->node2().data()->nodeName()));
    if(directed_)
    {
        indegree_[edge.data()->node2().data()->nodeName()] += 1;
    }
    else
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
        THROW(ArgumentException, "Invalid argument: Non-existing node");
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
bool Graph<NodeType, EdgeType>::addEdge(const QString& name_1, const NodeType& node_content_1,
                                        const QString& name_2, const NodeType& node_content_2,
                                        const EdgeType& edge_content)
{
    // only create and add nodes if they do not yet exist
    if(!hasNode(name_1))
    {
        addNode(name_1, node_content_1);
    }
    if(!hasNode(name_2))
    {
        addNode(name_2, node_content_2);
    }

    if(!hasEdge(name_1, name_2))
    {
        auto node_1 = getNode(name_1);
        auto node_2 = getNode(name_2);
        EdgePointer edge(new GraphEdge<EdgeType, NodeType>(node_1, node_2, edge_content));
        adjacency_list_[node_1].append(edge);
        edge_list_.insert(QPair<QString, QString>(name_1, name_2));
        indegree_[name_2] += 1;
        if(!directed_)
        {
            adjacency_list_[node_2].append(edge);
            indegree_[name_1] += 1;
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
    return hasEdge(node_1.data()->nodeName(), node_2.data()->nodeName());
}

// check if a specific edge exists by the names of the nodes that it connects
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::hasEdge(const QString& node_name_1, const QString& node_name_2)
{
    if(!hasNode(node_name_1) || !hasNode(node_name_2))
    {
        return false;
    }
    if(edge_list_.contains(QPair<QString, QString>(node_name_1, node_name_2))) {
        return true;
    }
    else if(edge_list_.contains(QPair<QString, QString>(node_name_2, node_name_1)) && !directed())
    {
        return true;
    }
    return false;
}

// get an edge from the two nodes that it connects
template <typename NodeType, typename EdgeType>
typename Graph<NodeType, EdgeType>::EdgePointer Graph<NodeType, EdgeType>::getEdge(NodePointer node_1, NodePointer node_2)
{
    if(!hasNode(node_1) || !hasNode(node_2))
    {
        THROW(ArgumentException, "Invalid argument: Non-existing node");
    }
    foreach(EdgePointer edge, adjacency_list_[node_1])
    {
        if(edge.data()->node1() == node_1 && edge.data()->node2() == node_2) {
            return edge;
        }
        else if(!directed_ && edge.data()->node1() == node_2 && edge.data()->node2() == node_1)
        {
            return edge;
        }
    }
    THROW(ArgumentException, "Invalid argument: Non-existing edge");
}

// get an edge from the names of the two nodes it connects
template <typename NodeType, typename EdgeType>
typename Graph<NodeType, EdgeType>::EdgePointer Graph<NodeType, EdgeType>::getEdge(const QString &node_name_1, const QString &node_name_2)
{
    if(!hasNode(node_name_1) || !hasNode(node_name_2))
    {
        THROW(ArgumentException, "Invalid argument: Non-existing node");
    }
    foreach(EdgePointer edge, adjacency_list_[getNode(node_name_1)])
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
    THROW(ArgumentException, "Invalid argument: Non-existing edge");
}

template <typename NodeType, typename EdgeType>
QList<typename Graph<NodeType, EdgeType>::NodePointer> Graph<NodeType, EdgeType>::getAdjacentNodes(const QString& name)
{
    QList<NodePointer> adjacent_nodes;
    if(!hasNode(name))
    {
        THROW(ArgumentException, "Illegal argument: Non-existing node");
    }
    foreach(EdgePointer edge, getAdjacentEdges(name))
    {
        if(edge.data()->node1().data()->nodeName() == name)
        {
            adjacent_nodes.append(edge.data()->node2());
        }
        else
        {
            adjacent_nodes.append(edge.data()->node1());
        }
    }
    return adjacent_nodes;
}

template <typename NodeType, typename EdgeType>
QList<typename Graph<NodeType, EdgeType>::EdgePointer> Graph<NodeType, EdgeType>::getAdjacentEdges(const QString& name)
{
    if(hasNode(name))
    {
        return adjacency_list_[getNode(name)];
    }
    else
    {
        THROW(ArgumentException, "Invalid argument: Non-existing node");
    }
}

// check if node with name_2 is adjacent to node with name_1
template <typename NodeType, typename EdgeType>
bool Graph<NodeType, EdgeType>::isAdjacent(const QString& name_1, const QString& name_2)
{
    if(!hasNode(name_1) || !hasNode(name_2))
    {
        THROW(ArgumentException, "Invalid argument: Non-existing node");
    }
    foreach(NodePointer node, getAdjacentNodes(name_1))
    {
        if(node.data()->nodeName() == name_2)
        {
            return true;
        }
    }
    return false;
}

// get the degree of a node
template <typename NodeType, typename EdgeType>
int Graph<NodeType, EdgeType>::getDegree(const QString& name)
{
    if(directed_)
    {
        THROW(Exception, "Invalid use of method: Only for undirected graphs");
    }
    if(hasNode(name))
    {
        return adjacency_list_[getNode(name)].size();
    }
    else
    {
        THROW(ArgumentException, "Invalid argument: Non-existing node");
    }
}

// for directed graphs only: get the indegree/outdegree of a node
template <typename NodeType, typename EdgeType>
int Graph<NodeType, EdgeType>::getIndegree(const QString& name)
{
    if(directed_)
    {
        if(hasNode(name))
        {
            return indegree_[name];
        }
        else
        {
            THROW(ArgumentException, "Invalid argument: Non-existing node")
        }
    }
    else
    {
        THROW(Exception, "Invalid use of method: Only for directed graphs");
    }
}

template <typename NodeType, typename EdgeType>
int Graph<NodeType, EdgeType>::getOutdegree(const QString& name)
{
    if(directed_)
    {
        if(hasNode(name))
        {
            return adjacency_list_[getNode(name)].size();
        }
        else
        {
            THROW(ArgumentException, "Invalid argument: Non-existing node")
        }
    }
    else
    {
        THROW(Exception, "Invalid use of method: Only for directed graphs");
    }
}

// write all edges to tsv file (as pair of nodes)
template <typename NodeType, typename EdgeType>
void Graph<NodeType, EdgeType>::store(const QString& file)
{
    QSharedPointer<QFile> writer = Helper::openFileForWriting(file);
	QTextStream stream(writer.data());
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
    #else
    stream.setCodec("UTF-8");
    #endif

    QList<QPair<QString, QString>> sorted_edge_list = edge_list_.values();

    std::sort(sorted_edge_list.begin(), sorted_edge_list.end(),
              [](QPair<QString, QString> a, QPair<QString, QString> b)
                {
                    if(a.first == b.first)
                    {
                        return a.second < b.second;
                    }
                    return a.first < b.first;
                });

    QPair<QString, QString> node_pair;
    foreach(node_pair, sorted_edge_list)
    {
        stream << node_pair.first << "\t" << node_pair.second << QT_ENDL;
    }
}


#endif // GRAPH_H
