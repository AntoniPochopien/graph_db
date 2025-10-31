#include "graph_db_c_api.h"
#include "storage.hpp"
#include "node.hpp"
#include "edge.hpp"

#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <cstddef>      // dla size_t
#include <stdexcept>
#include "json.hpp"     // nlohmann::json

using namespace graphdb;
using namespace std;
using json = nlohmann::json;

// =====================================
// Wewnętrzne funkcje C++ (nie w extern "C")
// =====================================
static vector<Node> parse_nodes_from_json(const char* jsonData)
{
    vector<Node> nodes;
    if (!jsonData)
        return nodes;

    json j = json::parse(jsonData);
    if (!j.is_array())
        throw runtime_error("Expected JSON array of nodes");

    for (const auto& item : j)
    {
        string nodeStr = item.dump();
        Node n = Node::from_json(nodeStr);
        nodes.push_back(std::move(n));
    }

    return nodes;
}

static vector<Edge> parse_edges_from_json(const char* jsonData)
{
    vector<Edge> edges;
    if (!jsonData)
        return edges;

    json j = json::parse(jsonData);
    if (!j.is_array())
        throw runtime_error("Expected JSON array of edges");

    for (const auto& item : j)
    {
        string edgeStr = item.dump();
        Edge e = Edge::from_json(edgeStr);
        edges.push_back(std::move(e));
    }

    return edges;
}

// =====================================
// Struktura przechowująca Storage
// =====================================
struct GraphDB
{
    unique_ptr<Storage> storage;
};

// =====================================
// C API
// =====================================
extern "C"
{

Box* graphdb_init(const char* boxName)
{
    if (!boxName)
        return nullptr;

    auto* box = new GraphDB();
    box->storage = make_unique<Storage>(string(boxName));
    box->storage->buildNodeIndex();
    box->storage->buildEdgeIndex();

    return box;
}

void graphdb_save_nodes(Box* box, const char* jsonData)
{
    if (!box || !jsonData)
        std::cerr << "graphdb_save_nodes: Error - box or jsonData is NULL.\n";
        return;

    try
    {
        vector<Node> nodes = parse_nodes_from_json(jsonData);
        std::cerr << "graphdb_save_nodes: Successfully parsed " << nodes.size() << " nodes.\n";
        box->storage->saveNodeChunk(nodes);
        std::cerr << "graphdb_save_nodes: saveNodeChunk finished successfully.\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "graphdb_save_nodes: CRITICAL ERROR - Exception caught: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "graphdb_save_nodes: CRITICAL ERROR - Unknown exception caught.\n";
    }
}

void graphdb_save_edges(Box* box, const char* jsonData)
{
    if (!box || !jsonData)
        return;

    try
    {
        vector<Edge> edges = parse_edges_from_json(jsonData);
        box->storage->saveEdgeChunk(edges);
    }
    catch (...)
    {
        // możesz tu dodać logowanie
    }
}

const char* graphdb_load_node(Box* box, const char* nodeId)
{
    if (!box || !nodeId)
        return nullptr;

    try
    {
        Node node = box->storage->loadNodeById(nodeId);
        string jsonStr = node.to_json();
        char* result = (char*)malloc(jsonStr.size() + 1);
        strcpy(result, jsonStr.c_str());
        return result;
    }
    catch (...)
    {
        return nullptr;
    }
}

const char* graphdb_load_edges(Box* box, const char* nodeId)
{
    if (!box || !nodeId)
        return nullptr;

    try
    {
        vector<Edge> edges = box->storage->loadEdgesFromNode(nodeId);
        json j = json::array();
        for (auto& e : edges)
            j.push_back(json::parse(e.to_json()));

        string jsonStr = j.dump();
        char* result = (char*)malloc(jsonStr.size() + 1);
        strcpy(result, jsonStr.c_str());
        return result;
    }
    catch (...)
    {
        return nullptr;
    }
}

void graphdb_build_node_index(Box* box)
{
    if (box)
        box->storage->buildNodeIndex();
}

void graphdb_build_edge_index(Box* box)
{
    if (box)
        box->storage->buildEdgeIndex();
}

void graphdb_free_string(const char* str)
{
    free((void*)str);
}

void graphdb_close(Box* box)
{
    delete box;
}

} // extern "C"