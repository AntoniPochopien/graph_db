#include <cstddef>
#ifndef GRAPH_DB_C_API_H
#define GRAPH_DB_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GraphDB Box;

// Initialize the storage (creates Storage instance)
Box* graphdb_init(const char* boxName);

// Save nodes / edges from JSON string
void graphdb_save_nodes(Box* box, const char* jsonData);
void graphdb_save_edges(Box* box, const char* jsonData);

// Delete a node by ID
void graphdb_delete_node(Box* box, const char* nodeId);

// Load a single node by ID (returns malloc'ed string, free with graphdb_free_string)
const char* graphdb_load_node(Box* box, const char* nodeId);

// Load edges for a node (returns malloc'ed JSON string)
const char* graphdb_load_edges(Box* box, const char* nodeId);

// Build indexes manually (optional, usually called internally)
void graphdb_build_node_index(Box* box);
void graphdb_build_edge_index(Box* box);

// Estimate node chunk size from JSON (returns size in bytes)
size_t graphdb_estimate_nodes_size(Box* box, const char* jsonData);

// Free memory returned by load_node/load_edges
void graphdb_free_string(const char* str);

#ifdef __cplusplus
}
#endif

#endif // GRAPH_DB_C_API_H
