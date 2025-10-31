#include "storage.hpp"
#include "node.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

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
    cerr << "Storage constructor: Initializing storage at base path: " << basePath << "\n";

    try {
        if (!fs::exists(NODES_BASE_PATH)) {
            cerr << "Storage constructor: Creating nodes directory: " << NODES_BASE_PATH << "\n";
            fs::create_directories(NODES_BASE_PATH);
        }
        if (!fs::exists(EDGES_BASE_PATH)) {
            cerr << "Storage constructor: Creating edges directory: " << EDGES_BASE_PATH << "\n";
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
                        catch (...) { cerr << "Warning: bad filename format in " << entry.path() << "\n"; }
                    }
                }
            } catch (const fs::filesystem_error& e) {
                 cerr << "Storage constructor: FILESYSTEM ERROR during directory iteration: " << e.what() << "\n";
            }
           

            if (lastIdx < 0) lastIdx = 0;
            cerr << "Storage constructor: Last index for " << prefix << " set to " << lastIdx << "\n";
        };

        initFolder(NODES_BASE_PATH, "nodes", lastNodeChunkIdx);
        initFolder(EDGES_BASE_PATH, "edges", lastEdgeChunkIdx);
        cerr << "Storage constructor: Initialization finished successfully.\n";
        
    } catch (const fs::filesystem_error& e) {
        cerr << "Storage constructor: CRITICAL FILESYSTEM ERROR during setup: " << e.what() << "\n";
        throw;
    }
}

// ====================== SAVE NODE CHUNK ======================
void Storage::saveNodeChunk(const vector<Node> &nodes)
{
    if (nodes.empty())
        return;
        
    // Log start
    std::cerr << "saveNodeChunk: Attempting to save " << nodes.size() << " nodes.\n";

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
            std::cerr << "saveNodeChunk: Appending to existing file: " << nextFile << "\n";
        }
    }

    fs::path targetFile;
    if (createNewChunk)
    {
        lastNodeChunkIdx++;
        // 🚨 CRITICAL LOGIC FIX
        // Should use the new index (lastNodeChunkIdx), not the old one (lastNodeChunkIdx - 1).
        targetFile = fs::path(NODES_BASE_PATH) / ("nodes_" + to_string(lastNodeChunkIdx) + ".bin");
        std::cerr << "saveNodeChunk: Creating NEW chunk with index " << lastNodeChunkIdx << ". File: " << targetFile << "\n";
    } else {
        targetFile = nextFile;
    }

    // 2. File opening
    ofstream out(targetFile, ios::binary | (createNewChunk ? ios::trunc : ios::app));
    
    // Using .is_open() for a precise check
    if (!out.is_open()) 
    {
        std::cerr << "saveNodeChunk: CRITICAL ERROR - Cannot open file for writing: " << targetFile << ". Check permissions and path.\n";
        return;
    }
    
    std::cerr << "saveNodeChunk: File opened successfully for " << (createNewChunk ? "TRUNCATE" : "APPEND") << " mode.\n";

    if (!createNewChunk)
    {
        // 3. APPEND (Append) Logic
        // ... (Code to read and update the count)
        
        // Reopening for read and write is risky, but if it works, we leave it.
        // Correct count update in the file is necessary.
        size_t oldCount;
        ifstream in(targetFile, ios::binary);
        if (!in.read(reinterpret_cast<char *>(&oldCount), sizeof(oldCount))) {
             std::cerr << "saveNodeChunk: Error reading old count from file.\n";
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
    
    // Replaced cout with cerr so logs are visible in Logcat
    std::cerr << "saveNodeChunk: SUCCESS - Wrote " << nodes.size() << " nodes to " << targetFile << "\n";
}

// ====================== Save edges chunk ======================
void Storage::saveEdgeChunk(const vector<Edge> &edges)
{
    fs::path filepath = fs::path(EDGES_BASE_PATH) / ("edges_" + to_string(lastEdgeChunkIdx) + ".bin");

    ofstream out(filepath, ios::binary);
    if (!out)
    {
        cerr << "Cannot open file for writing edges: " << filepath << "\n";
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
    cout << "Saved " << edgeCount << " edges to " << filepath << "\n";
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
        cerr << "Folder does not exist: " << folder << "\n";
        return;
    }

    for (const auto &entry : fs::directory_iterator(folder))
    {
        if (entry.path().extension() != ".bin")
            continue;

        ifstream in(entry.path(), ios::binary);
        if (!in)
        {
            cerr << "Cannot open file: " << entry.path() << "\n";
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

    cout << "Built node index for " << nodeIndex.size() << " NodeIDs\n";
}

// ====================== BUILD EDGE INDEX ======================
void Storage::buildEdgeIndex()
{
    edgeIndex.clear();

    fs::path folder = fs::path(EDGES_BASE_PATH);

    if (!exists(folder))
    {
        cerr << "Folder does not exist: " << folder << "\n";
        return;
    }

    for (const auto &entry : fs::directory_iterator(folder))
    {
        if (entry.path().extension() != ".bin")
            continue;

        ifstream in(entry.path(), ios::binary);
        if (!in)
        {
            cerr << "Cannot open file: " << entry.path() << "\n";
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

    cout << "Built edge index for " << edgeIndex.size() << " source nodes\n";
}