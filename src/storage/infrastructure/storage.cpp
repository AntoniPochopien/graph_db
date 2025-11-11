#include "storage.hpp"
#include "node.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>

using namespace std;
using namespace graphdb;

namespace fs = filesystem;

// Constructor (Box init)
Storage::Storage(const string &basePath) 
    : boxName(basePath), 
      lastNodeChunkIdx(0), 
      lastEdgeChunkIdx(0),
      NODES_BASE_PATH(fs::path(basePath) / "nodes"),
      EDGES_BASE_PATH(fs::path(basePath) / "edges")
{
    // Logowanie rozpoczęcia inicjalizacji
    printf("Storage constructor: Initializing storage at base path: %s\n", basePath.c_str());
    fflush(stdout);

    try {
        if (!fs::exists(NODES_BASE_PATH)) {
            printf("Storage constructor: Creating nodes directory: %s\n", NODES_BASE_PATH.c_str());
            fflush(stdout);
            fs::create_directories(NODES_BASE_PATH);
        }
        if (!fs::exists(EDGES_BASE_PATH)) {
            printf("Storage constructor: Creating edges directory: %s\n", EDGES_BASE_PATH.c_str());
            fflush(stdout);
            fs::create_directories(EDGES_BASE_PATH);
        }

        auto initFolder = [](const string &folderPath, const string &prefix, int &lastIdx)
        {
            lastIdx = -1;
            try {
                 for (const auto &entry : fs::directory_iterator(folderPath))
                {
                    auto name = entry.path().filename().string();
                    if (name.rfind(prefix + "_", 0) == 0 && name.find(".bin") != string::npos)
                    {
                        try
                        {
                            size_t idx = stoi(name.substr(prefix.size() + 1, name.size() - prefix.size() - 5));
                            if (idx > static_cast<size_t>(lastIdx))
                                lastIdx = static_cast<int>(idx);
                        }
                        catch (...) { printf("Warning: bad filename format in %s\n", entry.path().string().c_str()); fflush(stdout); }
                    }
                }
            } catch (const fs::filesystem_error& e) {
                 printf("Storage constructor: FILESYSTEM ERROR during directory iteration: %s\n", e.what());
                 fflush(stdout);
            }
           

            if (lastIdx < 0) lastIdx = 0;
            printf("Storage constructor: Last index for %s set to %d\n", prefix.c_str(), lastIdx);
            fflush(stdout);
        };

        initFolder(NODES_BASE_PATH, "nodes", lastNodeChunkIdx);
        initFolder(EDGES_BASE_PATH, "edges", lastEdgeChunkIdx);
        printf("Storage constructor: Initialization finished successfully.\n");
        fflush(stdout);
        
    } catch (const fs::filesystem_error& e) {
        printf("Storage constructor: CRITICAL FILESYSTEM ERROR during setup: %s\n", e.what());
        fflush(stdout);
        throw;
    }
}

// ====================== DELETE NODE ======================
void Storage::deleteNode(const string &nodeId)
{
    auto it = nodeIndex.find(nodeId);
    if (it == nodeIndex.end())
    {
        // Node doesn't exist, nothing to delete
        printf("deleteNode: Node %s not found in index, skipping deletion.\n", nodeId.c_str());
        fflush(stdout);
        return;
    }

    const string &filePath = it->second.first;
    
    printf("deleteNode: Attempting to delete node %s from file %s\n", nodeId.c_str(), filePath.c_str());
    fflush(stdout);
    
    // Read the entire file
    ifstream in(filePath, ios::binary);
    if (!in)
    {
        printf("deleteNode: Cannot open file for reading: %s\n", filePath.c_str());
        fflush(stdout);
        return;
    }

    size_t nodeCount;
    if (!in.read(reinterpret_cast<char *>(&nodeCount), sizeof(nodeCount)))
    {
        printf("deleteNode: Error reading node count from file: %s\n", filePath.c_str());
        fflush(stdout);
        in.close();
        return;
    }

    vector<Node> nodes;
    size_t offset = sizeof(nodeCount);

    // Read all nodes except the one to delete
    for (size_t i = 0; i < nodeCount; ++i)
    {
        size_t idLen;
        size_t nodeStartOffset = offset;
        if (!in.read(reinterpret_cast<char *>(&idLen), sizeof(idLen)))
        {
            printf("deleteNode: Error reading id length at offset %zu\n", offset);
            fflush(stdout);
            break;
        }

        string id(idLen, '\0');
        if (!in.read(&id[0], idLen))
        {
            printf("deleteNode: Error reading id at offset %zu\n", offset);
            fflush(stdout);
            break;
        }

        size_t propCount;
        if (!in.read(reinterpret_cast<char *>(&propCount), sizeof(propCount)))
        {
            printf("deleteNode: Error reading property count for node %s\n", id.c_str());
            fflush(stdout);
            break;
        }

        PropertyMap properties;
        for (size_t j = 0; j < propCount; ++j)
        {
            size_t klen;
            if (!in.read(reinterpret_cast<char *>(&klen), sizeof(klen)))
            {
                printf("deleteNode: Error reading key length for property %zu of node %s\n", j, id.c_str());
                fflush(stdout);
                break;
            }
            string key(klen, '\0');
            if (!in.read(&key[0], klen))
            {
                printf("deleteNode: Error reading key for property %zu of node %s\n", j, id.c_str());
                fflush(stdout);
                break;
            }

            PropertyValue val = PropertyValue::deserialize(in);
            properties[key] = val;
        }

        // Only keep nodes that are NOT being deleted
        if (id != nodeId)
        {
            Node node;
            node.id = id;
            node.properties = properties;
            nodes.push_back(node);
        }
        else
        {
            printf("deleteNode: Found node %s to delete, skipping it.\n", id.c_str());
            fflush(stdout);
        }

        offset = in.tellg();
    }
    in.close();

    // Rewrite the file with remaining nodes
    ofstream out(filePath, ios::binary | ios::trunc);
    if (!out.is_open())
    {
        printf("deleteNode: Cannot open file for writing: %s\n", filePath.c_str());
        fflush(stdout);
        return;
    }

    size_t newCount = nodes.size();
    out.write(reinterpret_cast<const char *>(&newCount), sizeof(newCount));

    for (const auto &node : nodes)
    {
        size_t len = node.id.size();
        out.write(reinterpret_cast<const char *>(&len), sizeof(len));
        out.write(node.id.c_str(), len);

        size_t propCount = node.properties.size();
        out.write(reinterpret_cast<const char *>(&propCount), sizeof(propCount));

        for (const auto &[key, value] : node.properties)
        {
            size_t klen = key.size();
            out.write(reinterpret_cast<const char *>(&klen), sizeof(klen));
            out.write(key.c_str(), klen);

            value.serialize(out);
        }
    }
    out.close();

    printf("deleteNode: Successfully deleted node %s from %s. Remaining nodes: %zu\n", nodeId.c_str(), filePath.c_str(), newCount);
    fflush(stdout);
}

// ====================== SAVE NODE CHUNK ======================
void Storage::saveNodeChunk(const vector<Node> &nodes)
{
    if (nodes.empty())
        return;
        
    // Log start
    printf("saveNodeChunk: Attempting to save %zu nodes.\n", nodes.size());
    fflush(stdout);
    
    // Delete any existing nodes with the same IDs to avoid duplicates and wasted space
    for (const auto &node : nodes)
    {
        if (nodeIndex.find(node.id) != nodeIndex.end())
        {
            printf("saveNodeChunk: Node %s already exists, deleting old version first.\n", node.id.c_str());
            fflush(stdout);
            deleteNode(node.id);
        }
    }
    
    // Rebuild index after deletions to ensure it's up to date
    buildNodeIndex();

    // 1. Filename for the next potential chunk
    fs::path nextFile = fs::path(NODES_BASE_PATH) / ("nodes_" + to_string(lastNodeChunkIdx + 1) + ".bin");

    bool createNewChunk = true;
    size_t newDataSize = estimateNodesSize(nodes);

    if (fs::exists(nextFile))
    {
        auto currentSize = fs::file_size(nextFile);
        if (currentSize + newDataSize <= MAX_CHUNK_SIZE)
        {
            createNewChunk = false;
            printf("saveNodeChunk: Appending to existing file: %s\n", nextFile.string().c_str());
            fflush(stdout);
        }
    }

    fs::path targetFile;
    if (createNewChunk)
    {
        lastNodeChunkIdx++;
        // 🚨 CRITICAL LOGIC FIX
        // Should use the new index (lastNodeChunkIdx), not the old one (lastNodeChunkIdx - 1).
        targetFile = fs::path(NODES_BASE_PATH) / ("nodes_" + to_string(lastNodeChunkIdx) + ".bin");
        printf("saveNodeChunk: Creating NEW chunk with index %d. File: %s\n", lastNodeChunkIdx, targetFile.string().c_str());
        fflush(stdout);
    } else {
        targetFile = nextFile;
    }

    // 2. File opening
    ofstream out(targetFile, ios::binary | (createNewChunk ? ios::trunc : ios::app));
    
    // Using .is_open() for a precise check
    if (!out.is_open()) 
    {
        printf("saveNodeChunk: CRITICAL ERROR - Cannot open file for writing: %s. Check permissions and path.\n", targetFile.string().c_str());
        fflush(stdout);
        return;
    }
    
    printf("saveNodeChunk: File opened successfully for %s mode.\n", createNewChunk ? "TRUNCATE" : "APPEND");
    fflush(stdout);

    if (!createNewChunk)
    {
        // 3. APPEND (Append) Logic
        // ... (Code to read and update the count)
        
        // Reopening for read and write is risky, but if it works, we leave it.
        // Correct count update in the file is necessary.
        size_t oldCount;
        ifstream in(targetFile, ios::binary);
        if (!in.read(reinterpret_cast<char *>(&oldCount), sizeof(oldCount))) {
             printf("saveNodeChunk: Error reading old count from file.\n");
             fflush(stdout);
             return;
        }
        in.close();

        size_t newCount = oldCount + nodes.size();
        
        // Open the file for reading and writing
        fstream updateCount(targetFile, ios::binary | ios::in | ios::out);
        
        // Write the new count at the beginning of the file
        updateCount.write(reinterpret_cast<const char *>(&newCount), sizeof(newCount));
        
        // Seek to the end of the file to append new nodes
        updateCount.seekp(0, ios::end); 
        
        for (const auto &node : nodes)
        {
            // ... (node data write logic)
            size_t len = node.id.size();
            updateCount.write(reinterpret_cast<const char *>(&len), sizeof(len));
            updateCount.write(node.id.c_str(), len);

            size_t propCount = node.properties.size();
            updateCount.write(reinterpret_cast<const char *>(&propCount), sizeof(propCount));

            for (const auto &[key, value] : node.properties)
            {
                size_t klen = key.size();
                updateCount.write(reinterpret_cast<const char *>(&klen), sizeof(klen));
                updateCount.write(key.c_str(), klen);

                value.serialize(updateCount);
            }
        }
        updateCount.close();
        
        // Replaced cout message at the end:
        // cout << "Saved " << nodes.size() << " nodes to " << targetFile << "\n"; 
        
    }
    else
    {
        // 4. TRUNCATE Logic (new file)
        
        // write new file
        size_t nodeCount = nodes.size();
        out.write(reinterpret_cast<const char *>(&nodeCount), sizeof(nodeCount));

        for (const auto &node : nodes)
        {
            size_t len = node.id.size();
            out.write(reinterpret_cast<const char *>(&len), sizeof(len));
            out.write(node.id.c_str(), len);

            size_t propCount = node.properties.size();
            out.write(reinterpret_cast<const char *>(&propCount), sizeof(propCount));

            for (const auto &[key, value] : node.properties)
            {
                size_t klen = key.size();
                out.write(reinterpret_cast<const char *>(&klen), sizeof(klen));
                out.write(key.c_str(), klen);

                value.serialize(out);
            }
        }
        out.close();
    }
    
    // Using printf for better cross-platform logging
    printf("saveNodeChunk: SUCCESS - Wrote %zu nodes to %s\n", nodes.size(), targetFile.string().c_str());
    fflush(stdout);
}

// ====================== Save edges chunk ======================
void Storage::saveEdgeChunk(const vector<Edge> &edges)
{
    fs::path filepath = fs::path(EDGES_BASE_PATH) / ("edges_" + to_string(lastEdgeChunkIdx) + ".bin");

    ofstream out(filepath, ios::binary);
    if (!out)
    {
        printf("saveEdgeChunk: Cannot open file for writing edges: %s\n", filepath.string().c_str());
        fflush(stdout);
        return;
    }

    size_t edgeCount = edges.size();
    out.write(reinterpret_cast<const char *>(&edgeCount), sizeof(edgeCount));

    for (const auto &e : edges)
    {
        // from
        size_t lenFrom = e.from.size();
        out.write(reinterpret_cast<const char *>(&lenFrom), sizeof(lenFrom));
        out.write(e.from.c_str(), lenFrom);

        // to
        size_t lenTo = e.to.size();
        out.write(reinterpret_cast<const char *>(&lenTo), sizeof(lenTo));
        out.write(e.to.c_str(), lenTo);

        // weight
        out.write(reinterpret_cast<const char *>(&e.weight), sizeof(e.weight));

        // properties
        size_t propCount = e.properties.size();
        out.write(reinterpret_cast<const char *>(&propCount), sizeof(propCount));
        for (const auto &[k, v] : e.properties)
        {
            size_t klen = k.size();
            out.write(reinterpret_cast<const char *>(&klen), sizeof(klen));
            out.write(k.c_str(), klen);
            v.serialize(out);
        }
    }

    out.close();
    printf("saveEdgeChunk: Saved %zu edges to %s\n", edgeCount, filepath.string().c_str());
    fflush(stdout);
}

// ====================== ESTIMATE NODES SIZE ======================
size_t Storage::estimateNodesSize(const vector<Node> &nodes)
{
    size_t total = 0;
    for (const auto &n : nodes)
    {
        total += sizeof(size_t) + n.id.size();
        total += sizeof(size_t);
        for (const auto &[k, v] : n.properties)
        {
            total += sizeof(size_t) + k.size();
            total += v.estimateSize();
        }
    }
    return total;
}

// ====================== LOAD NODE BY ID ======================
Node Storage::loadNodeById(const string &nodeId)
{
    auto it = nodeIndex.find(nodeId);
    if (it == nodeIndex.end())
    {
        throw runtime_error("NodeID not found in index: " + nodeId);
    }

    const string &file = it->second.first;
    size_t offset = it->second.second;

    ifstream in(file, ios::binary);
    if (!in)
    {
        throw runtime_error("Cannot open file: " + file);
    }

    in.seekg(offset);

    size_t idLen;
    in.read(reinterpret_cast<char *>(&idLen), sizeof(idLen));
    string id(idLen, '\0');
    in.read(&id[0], idLen);

    size_t propCount;
    in.read(reinterpret_cast<char *>(&propCount), sizeof(propCount));

    PropertyMap properties;
    for (size_t i = 0; i < propCount; ++i)
    {
        size_t klen;
        in.read(reinterpret_cast<char *>(&klen), sizeof(klen));
        string key(klen, '\0');
        in.read(&key[0], klen);

        PropertyValue val = PropertyValue::deserialize(in);
        properties[key] = val;
    }

    in.close();

    Node node;
    node.id = id;
    node.properties = properties;

    return node;
}

// ====================== Load edges from node ======================
vector<Edge> Storage::loadEdgesFromNode(const string &nodeId)
{
    vector<Edge> edges;
    auto it = edgeIndex.find(nodeId);
    if (it == edgeIndex.end())
        return edges;

    for (const auto &[file, offset] : it->second)
    {
        ifstream in(file, ios::binary);
        if (!in)
            continue;

        in.seekg(offset);

        Edge e;

        size_t lenFrom;
        in.read(reinterpret_cast<char *>(&lenFrom), sizeof(lenFrom));
        e.from.resize(lenFrom);
        in.read(&e.from[0], lenFrom);

        size_t lenTo;
        in.read(reinterpret_cast<char *>(&lenTo), sizeof(lenTo));
        e.to.resize(lenTo);
        in.read(&e.to[0], lenTo);

        in.read(reinterpret_cast<char *>(&e.weight), sizeof(e.weight));

        size_t propCount;
        in.read(reinterpret_cast<char *>(&propCount), sizeof(propCount));
        for (size_t j = 0; j < propCount; ++j)
        {
            size_t klen;
            in.read(reinterpret_cast<char *>(&klen), sizeof(klen));
            string key(klen, '\0');
            in.read(&key[0], klen);

            PropertyValue val = PropertyValue::deserialize(in);
            e.properties[key] = val;
        }

        edges.push_back(e);
    }

    return edges;
}

// ====================== BUILD NODE INDEX ======================
void Storage::buildNodeIndex()
{
    nodeIndex.clear();

    fs::path folder = fs::path(NODES_BASE_PATH);

    if (!exists(folder))
    {
        printf("buildNodeIndex: Folder does not exist: %s\n", folder.string().c_str());
        fflush(stdout);
        return;
    }

    for (const auto &entry : fs::directory_iterator(folder))
    {
        if (entry.path().extension() != ".bin")
            continue;

        ifstream in(entry.path(), ios::binary);
        if (!in)
        {
            printf("buildNodeIndex: Cannot open file: %s\n", entry.path().string().c_str());
            fflush(stdout);
            continue;
        }

        size_t nodeCount;
        in.read(reinterpret_cast<char *>(&nodeCount), sizeof(nodeCount));

        size_t offset = sizeof(nodeCount);

        for (size_t i = 0; i < nodeCount; ++i)
        {
            size_t idLen;
            size_t nodeStartOffset = offset; // <-- początek węzła
            in.read(reinterpret_cast<char *>(&idLen), sizeof(idLen));

            string id(idLen, '\0');
            in.read(&id[0], idLen);

            size_t propCount;
            in.read(reinterpret_cast<char *>(&propCount), sizeof(propCount));

            for (size_t j = 0; j < propCount; ++j)
            {
                size_t keyLen;
                in.read(reinterpret_cast<char *>(&keyLen), sizeof(keyLen));
                in.seekg(keyLen, ios::cur);

                PropertyValue val = PropertyValue::deserialize(in);
            }

            nodeIndex[id] = {entry.path().string(), nodeStartOffset};

            offset = in.tellg();
        }

        in.close();
    }

    printf("Built node index for %zu NodeIDs\n", nodeIndex.size());
    fflush(stdout);
}

// ====================== BUILD EDGE INDEX ======================
void Storage::buildEdgeIndex()
{
    edgeIndex.clear();

    fs::path folder = fs::path(EDGES_BASE_PATH);

    if (!exists(folder))
    {
        printf("buildEdgeIndex: Folder does not exist: %s\n", folder.string().c_str());
        fflush(stdout);
        return;
    }

    for (const auto &entry : fs::directory_iterator(folder))
    {
        if (entry.path().extension() != ".bin")
            continue;

        ifstream in(entry.path(), ios::binary);
        if (!in)
        {
            printf("buildEdgeIndex: Cannot open file: %s\n", entry.path().string().c_str());
            fflush(stdout);
            continue;
        }

        size_t edgeCount;
        in.read(reinterpret_cast<char *>(&edgeCount), sizeof(edgeCount));
        size_t offset = sizeof(edgeCount);

        for (size_t i = 0; i < edgeCount; ++i)
        {
            size_t startOffset = offset;

            // from
            size_t fromLen;
            in.read(reinterpret_cast<char *>(&fromLen), sizeof(fromLen));
            string from(fromLen, '\0');
            in.read(&from[0], fromLen);

            // to
            size_t toLen;
            in.read(reinterpret_cast<char *>(&toLen), sizeof(toLen));
            string to(toLen, '\0');
            in.read(&to[0], toLen);

            // weight
            double weight;
            in.read(reinterpret_cast<char *>(&weight), sizeof(weight));

            // properties
            size_t propCount;
            in.read(reinterpret_cast<char *>(&propCount), sizeof(propCount));
            for (size_t j = 0; j < propCount; ++j)
            {
                size_t keyLen;
                in.read(reinterpret_cast<char *>(&keyLen), sizeof(keyLen));
                in.seekg(keyLen, ios::cur);
                PropertyValue::deserialize(in);
            }

            edgeIndex[from].emplace_back(entry.path().string(), startOffset);

            offset = in.tellg();
        }

        in.close();
    }

    printf("Built edge index for %zu source nodes\n", edgeIndex.size());
    fflush(stdout);
}