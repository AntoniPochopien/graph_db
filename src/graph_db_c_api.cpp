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
#include <cstdio>       // for printf
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
    if (!box || !jsonData) {
        printf("graphdb_save_nodes: Error - box or jsonData is NULL.\n");
        fflush(stdout);
        return;
    }

    try
    {
        vector<Node> nodes = parse_nodes_from_json(jsonData);
        printf("graphdb_save_nodes: Successfully parsed %zu nodes.\n", nodes.size());
        fflush(stdout);
        box->storage->saveNodeChunk(nodes);
        printf("graphdb_save_nodes: saveNodeChunk finished successfully.\n");
        fflush(stdout);
        // Rebuild index after saving nodes so they can be loaded
        box->storage->buildNodeIndex();
        printf("graphdb_save_nodes: Node index rebuilt.\n");
        fflush(stdout);
    }
    catch (const std::exception& e)
    {
        printf("graphdb_save_nodes: CRITICAL ERROR - Exception caught: %s\n", e.what());
        fflush(stdout);
    }
    catch (...)
    {
        printf("graphdb_save_nodes: CRITICAL ERROR - Unknown exception caught.\n");
        fflush(stdout);
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
    }
}

void graphdb_delete_node(Box* box, const char* nodeId)
{
    if (!box || !nodeId) {
        printf("graphdb_delete_node: Error - box or nodeId is NULL.\n");
        fflush(stdout);
        return;
    }

    try
    {
        box->storage->deleteNode(string(nodeId));
        box->storage->buildNodeIndex();
        printf("graphdb_delete_node: Successfully deleted node: %s\n", nodeId);
        fflush(stdout);
    }
    catch (const std::exception& e)
    {
        printf("graphdb_delete_node: ERROR - Exception caught: %s\n", e.what());
        fflush(stdout);
    }
    catch (...)
    {
        printf("graphdb_delete_node: ERROR - Unknown exception caught.\n");
        fflush(stdout);
    }
}

const char* graphdb_load_node(Box* box, const char* nodeId)
{
    if (!box || !nodeId) {
        printf("graphdb_load_node: Error - box or nodeId is NULL.\n");
        fflush(stdout);
        return nullptr;
    }

    try
    {
        printf("graphdb_load_node: Attempting to load node with ID: %s\n", nodeId);
        fflush(stdout);
        Node node = box->storage->loadNodeById(nodeId);
        string jsonStr = node.to_json();
        printf("graphdb_load_node: Successfully loaded node, JSON size: %zu\n", jsonStr.size());
        fflush(stdout);
        char* result = (char*)malloc(jsonStr.size() + 1);
        strcpy(result, jsonStr.c_str());
        return result;
    }
    catch (const std::exception& e)
    {
        printf("graphdb_load_node: ERROR - Exception caught: %s\n", e.what());
        fflush(stdout);
        return nullptr;
    }
    catch (...)
    {
        printf("graphdb_load_node: ERROR - Unknown exception caught.\n");
        fflush(stdout);
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