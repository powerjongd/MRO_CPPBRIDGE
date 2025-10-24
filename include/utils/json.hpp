#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace mini_json {

class Value {
public:
    using Object = std::map<std::string, Value>;
    using Array = std::vector<Value>;

    Value();
    Value(std::nullptr_t);
    Value(bool b);
    Value(double num);
    Value(int num);
    Value(const char* str);
    Value(std::string str);
    Value(Object obj);
    Value(Array arr);

    bool is_null() const;
    bool is_bool() const;
    bool is_number() const;
    bool is_string() const;
    bool is_object() const;
    bool is_array() const;

    bool as_bool(bool fallback = false) const;
    double as_number(double fallback = 0.0) const;
    std::string as_string(const std::string& fallback = {}) const;

    const Object& as_object() const;
    const Array& as_array() const;

    Object& as_object();
    Array& as_array();

    std::string dump(int indent = 0) const;

private:
    std::string dump_impl(int indent, int level) const;

    std::variant<std::nullptr_t, bool, double, std::string, Object, Array> data_;
};

Value parse(const std::string& text);

}  // namespace mini_json
