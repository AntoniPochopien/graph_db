#pragma once
#include "node.hpp"
#include "edge.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

using namespace std;

namespace graphdb
{
    struct Graph
    {
        unordered_map<string, Node> nodes;
        unordered_map<string, vector<Edge>> adjacencyList;

        // CRUD Node
        bool addNode(const Node &node);
        optional<Node> getNode(const string &id);
        bool removeNode(const string &id);

        // CRUD Edge
        bool addEdge(const Edge &edge);
        optional<Edge> getEdge(const string &from, const string &to);
        bool removeEdge(const string &from, const string &to);

        // Utility
        vector<Node> getNodesPage(size_t start, size_t limit);
        vector<Edge> getNeighbors(const std::string &nodeId) const;

        vector<Node> getAllNodes() const;
        vector<Edge> getAllEdges() const;
    };
}