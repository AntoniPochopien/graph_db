#pragma once
#include <variant>
#include <unordered_map>
#include <string>
#include <memory>
#include <iostream>
#include "json.hpp"

using namespace std;
namespace graphdb
{
    struct PropertyValue;

    using PropertyMap = unordered_map<string, PropertyValue>;

    struct PropertyValue {
        using variant_type = variant<int, double, string, bool, shared_ptr<PropertyMap>>;
        variant_type value;

        //default
        PropertyValue() : value(0) {}

        PropertyValue(const PropertyMap& map) : value(make_shared<PropertyMap>(map)) {}
        PropertyValue(int v) : value(v) {}
        PropertyValue(double v) : value(v) {}
        PropertyValue(const string& v) : value(v) {}
        PropertyValue(bool v) : value(v) {}

        size_t estimateSize() const;

        void serialize(ostream& out) const;
        static PropertyValue deserialize(istream& in);

        nlohmann::json to_json() const;
        static PropertyValue from_json(const nlohmann::json& j);
    };
}