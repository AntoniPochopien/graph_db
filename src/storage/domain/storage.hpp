#pragma once
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include "node.hpp"
#include "edge.hpp"

using namespace std;
namespace fs = filesystem;

namespace graphdb
{
    class Storage
    {
    public:
        Storage(const string &basePath);
        ~Storage() = default;

        void saveNodeChunk(const vector<Node> &nodes);
        void saveEdgeChunk(const vector<Edge> &edges);

        void deleteNode(const string &nodeId);
        Node loadNodeById(const string &nodeId);
        vector<Edge> loadEdgesFromNode(const string &nodeId);

        void buildNodeIndex();
        void buildEdgeIndex();

        size_t estimateNodesSize(const vector<Node> &nodes);

    private:
        string boxName;
        unordered_map<string, pair<string, size_t>> nodeIndex;
        unordered_map<string, vector<pair<string, size_t>>> edgeIndex;
        int lastNodeChunkIdx;
        int lastEdgeChunkIdx;

        string NODES_BASE_PATH;
        string EDGES_BASE_PATH;
        static const size_t MAX_CHUNK_SIZE = 1 * 1024 * 1024;
    };
}