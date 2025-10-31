#pragma once
#include <string>
#include <unordered_map>
#include "property.hpp"

using namespace std;

namespace graphdb
{
    struct Edge
    {
        string from;
        string to;
        double weight;
        PropertyMap properties;

        void print() const;

        void serialize(ostream &out) const;
        static Edge deserialize(istream &in);

        string to_json() const;
        static Edge from_json(const string &jsonStr);
    };
}