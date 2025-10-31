#include "graph.hpp"
#include <algorithm>

using namespace std;
using namespace graphdb;

// Node
bool Graph::addNode(const Node &node)
{
    auto res = nodes.emplace(node.id, node);
    return res.second;
}

optional<Node> Graph::getNode(const string &id)
{
    auto it = nodes.find(id);
    if (it != nodes.end())
        return it->second;
    return nullopt;
}

bool Graph::removeNode(const string &id)
{
    auto erased = nodes.erase(id);
    adjacencyList.erase(id);

    for (auto &[_, edges] : adjacencyList)
    {
        edges.erase(remove_if(edges.begin(), edges.end(),
                              [&](const Edge &e)
                              { return e.to == id; }),
                    edges.end());
    }
    return erased > 0;
}

// Edge
bool Graph::addEdge(const Edge &edge)
{
    if (nodes.find(edge.from) == nodes.end() || nodes.find(edge.to) == nodes.end())
        return false;
    adjacencyList[edge.from].push_back(edge);
    return true;
}

optional<Edge> Graph::getEdge(const string &from, const string &to)
{
    auto it = adjacencyList.find(from);
    if (it == adjacencyList.end())
        return nullopt;
    for (const auto &e : it->second)
    {
        if (e.to == to)
            return e;
    }
    return nullopt;
}

bool Graph::removeEdge(const string &from, const string &to)
{
    auto it = adjacencyList.find(from);
    if (it == adjacencyList.end())
        return false;
    auto &edges = it->second;
    auto oldSize = edges.size();
    edges.erase(remove_if(edges.begin(), edges.end(),
                          [&](const Edge &e)
                          { return e.to == to; }),
                edges.end());
    return edges.size() != oldSize;
}

// Utility
vector<Node> Graph::getNodesPage(size_t start, size_t limit)
{
    vector<Node> page;
    size_t i = 0;
    for (auto &[_, node] : nodes)
    {
        if (i >= start && page.size() < limit)
            page.push_back(node);
        ++i;
    }
    return page;
}

vector<Edge> Graph::getNeighbors(const string &nodeId) const
{
    auto it = adjacencyList.find(nodeId);
    if (it != adjacencyList.end())
    {
        return it->second;
    }
    return {};
}

vector<Node> Graph::getAllNodes() const
{
    vector<Node> result;
    for (const auto &[id, node] : nodes)
        result.push_back(node);
    return result;
}

vector<Edge> Graph::getAllEdges() const
{
    vector<Edge> allEdges;

    for (const auto &pair : adjacencyList)
    {
        const auto &edges = pair.second;
        allEdges.insert(allEdges.end(), edges.begin(), edges.end());
    }

    return allEdges;
}
