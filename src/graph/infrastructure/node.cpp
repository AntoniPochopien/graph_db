#include "node.hpp"
#include <iostream>
#include "json.hpp"

using namespace std;
using namespace graphdb;
using json = nlohmann::json;

void Node::print() const {
    cout << "Node id: " << id << "\n";
    for (const auto& [k, v] : properties)
        cout << "  " << k << ": (property)\n";
}


string Node::to_json() const {
    json j;
    j["id"] = id;
    for (const auto& [key, val] : properties) {
        j["properties"][key] = val.to_json();
    }
    return j.dump();
}

Node Node::from_json(const string& jsonStr) {
    json j = json::parse(jsonStr);
    Node node;
    node.id = j["id"].get<string>();
    for (auto& [key, value] : j["properties"].items()) {
        node.properties[key] = PropertyValue::from_json(value);
    }
    return node;
}

void Node::serialize(ostream& out) const {
    size_t idLen = id.size();
    out.write(reinterpret_cast<const char*>(&idLen), sizeof(idLen));
    out.write(id.data(), idLen);

    size_t propCount = properties.size();
    out.write(reinterpret_cast<const char*>(&propCount), sizeof(propCount));

    for (const auto& [k, v] : properties) {
        size_t klen = k.size();
        out.write(reinterpret_cast<const char*>(&klen), sizeof(klen));
        out.write(k.data(), klen);
        v.serialize(out);
    }
}

Node Node::deserialize(istream& in) {
    Node node;

    size_t idLen;
    in.read(reinterpret_cast<char*>(&idLen), sizeof(idLen));
    node.id.resize(idLen);
    in.read(node.id.data(), idLen);

    size_t propCount;
    in.read(reinterpret_cast<char*>(&propCount), sizeof(propCount));

    for (size_t i = 0; i < propCount; ++i) {
        size_t klen;
        in.read(reinterpret_cast<char*>(&klen), sizeof(klen));
        string key(klen, '\0');
        in.read(key.data(), klen);

        PropertyValue val = PropertyValue::deserialize(in);
        node.properties.emplace(move(key), move(val));
    }

    return node;
}
