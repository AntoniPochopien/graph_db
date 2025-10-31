#pragma once
#include <any>
#include <string>
#include "property.hpp"

using namespace std;
using namespace graphdb;

namespace graphdb
{
    struct Node
    {
        string id;
        PropertyMap properties;

        void print() const;
        void serialize(ostream& out) const;
        static Node deserialize(istream& in);

        string to_json() const;
        static Node from_json(const string& jsonStr);
    };
}