#include "utils/json.hpp"

#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace mini_json {

namespace {

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    Value parse_value() {
        skip_ws();
        if (eof()) {
            throw std::runtime_error("Unexpected end of JSON input");
        }
        char ch = peek();
        if (ch == 'n') return parse_null();
        if (ch == 't' || ch == 'f') return parse_bool();
        if (ch == '"') return parse_string();
        if (ch == '{') return parse_object();
        if (ch == '[') return parse_array();
        if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch))) {
            return parse_number();
        }
        throw std::runtime_error("Invalid JSON token");
    }

    void ensure_consumed() {
        skip_ws();
        if (!eof()) {
            throw std::runtime_error("Extra data after JSON value");
        }
    }

private:
    Value parse_null() {
        expect("null");
        return Value(nullptr);
    }

    Value parse_bool() {
        if (match("true")) {
            return Value(true);
        }
        expect("false");
        return Value(false);
    }

    Value parse_number() {
        size_t start = pos_;
        if (peek() == '-') advance();
        while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
        if (!eof() && peek() == '.') {
            advance();
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }
        if (!eof() && (peek() == 'e' || peek() == 'E')) {
            advance();
            if (!eof() && (peek() == '+' || peek() == '-')) {
                advance();
            }
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }
        double value = std::stod(text_.substr(start, pos_ - start));
        return Value(value);
    }

    Value parse_string() {
        expect('"');
        std::ostringstream oss;
        while (!eof()) {
            char ch = advance();
            if (ch == '"') {
                return Value(oss.str());
            }
            if (ch == '\\') {
                if (eof()) {
                    throw std::runtime_error("Invalid escape sequence");
                }
                char esc = advance();
                switch (esc) {
                    case '\"': oss << '"'; break;
                    case '\\': oss << '\\'; break;
                    case '/': oss << '/'; break;
                    case 'b': oss << '\b'; break;
                    case 'f': oss << '\f'; break;
                    case 'n': oss << '\n'; break;
                    case 'r': oss << '\r'; break;
                    case 't': oss << '\t'; break;
                    case 'u': {
                        std::string hex;
                        for (int i = 0; i < 4; ++i) {
                            if (eof()) throw std::runtime_error("Invalid unicode escape");
                            hex.push_back(advance());
                        }
                        char16_t code = static_cast<char16_t>(std::stoi(hex, nullptr, 16));
                        if (code < 0x80) {
                            oss << static_cast<char>(code);
                        } else if (code < 0x800) {
                            oss << static_cast<char>(0xC0 | ((code >> 6) & 0x1F));
                            oss << static_cast<char>(0x80 | (code & 0x3F));
                        } else {
                            oss << static_cast<char>(0xE0 | ((code >> 12) & 0x0F));
                            oss << static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                            oss << static_cast<char>(0x80 | (code & 0x3F));
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("Unknown escape sequence");
                }
            } else {
                oss << ch;
            }
        }
        throw std::runtime_error("Unterminated string literal");
    }

    Value parse_array() {
        expect('[');
        Value::Array arr;
        skip_ws();
        if (peek() == ']') {
            advance();
            return Value(arr);
        }
        while (true) {
            arr.push_back(parse_value());
            skip_ws();
            if (peek() == ',') {
                advance();
                skip_ws();
                continue;
            }
            if (peek() == ']') {
                advance();
                break;
            }
            throw std::runtime_error("Expected ',' or ']'");
        }
        return Value(arr);
    }

    Value parse_object() {
        expect('{');
        Value::Object obj;
        skip_ws();
        if (peek() == '}') {
            advance();
            return Value(obj);
        }
        while (true) {
            skip_ws();
            if (peek() != '"') {
                throw std::runtime_error("Expected string key");
            }
            std::string key = parse_string().as_string();
            skip_ws();
            expect(':');
            skip_ws();
            obj.emplace(std::move(key), parse_value());
            skip_ws();
            if (peek() == ',') {
                advance();
                skip_ws();
                continue;
            }
            if (peek() == '}') {
                advance();
                break;
            }
            throw std::runtime_error("Expected ',' or '}' in object");
        }
        return Value(obj);
    }

    void skip_ws() {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    bool eof() const { return pos_ >= text_.size(); }
    char peek() const { return eof() ? '\0' : text_[pos_]; }
    char advance() { return text_[pos_++]; }

    bool match(const char* token) {
        size_t len = std::strlen(token);
        if (text_.compare(pos_, len, token) == 0) {
            pos_ += len;
            return true;
        }
        return false;
    }

    void expect(const char* token) {
        if (!match(token)) {
            throw std::runtime_error("Unexpected token in JSON input");
        }
    }

    void expect(char token) {
        if (eof() || text_[pos_] != token) {
            throw std::runtime_error("Unexpected character in JSON input");
        }
        ++pos_;
    }

    const std::string& text_;
    size_t pos_ = 0;
};

std::string escape_string(const std::string& s) {
    std::ostringstream oss;
    oss << '"';
    for (char ch : s) {
        switch (ch) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch));
                } else {
                    oss << ch;
                }
        }
    }
    oss << '"';
    return oss.str();
}

}  // namespace

Value::Value() : data_(nullptr) {}
Value::Value(std::nullptr_t) : data_(nullptr) {}
Value::Value(bool b) : data_(b) {}
Value::Value(double num) : data_(num) {}
Value::Value(int num) : data_(static_cast<double>(num)) {}
Value::Value(const char* str) : data_(std::string(str)) {}
Value::Value(std::string str) : data_(std::move(str)) {}
Value::Value(Object obj) : data_(std::move(obj)) {}
Value::Value(Array arr) : data_(std::move(arr)) {}

bool Value::is_null() const { return std::holds_alternative<std::nullptr_t>(data_); }
bool Value::is_bool() const { return std::holds_alternative<bool>(data_); }
bool Value::is_number() const { return std::holds_alternative<double>(data_); }
bool Value::is_string() const { return std::holds_alternative<std::string>(data_); }
bool Value::is_object() const { return std::holds_alternative<Object>(data_); }
bool Value::is_array() const { return std::holds_alternative<Array>(data_); }

bool Value::as_bool(bool fallback) const {
    if (auto ptr = std::get_if<bool>(&data_)) {
        return *ptr;
    }
    return fallback;
}

double Value::as_number(double fallback) const {
    if (auto ptr = std::get_if<double>(&data_)) {
        return *ptr;
    }
    return fallback;
}

std::string Value::as_string(const std::string& fallback) const {
    if (auto ptr = std::get_if<std::string>(&data_)) {
        return *ptr;
    }
    return fallback;
}

const Value::Object& Value::as_object() const {
    if (!is_object()) throw std::runtime_error("JSON value is not an object");
    return std::get<Object>(data_);
}

const Value::Array& Value::as_array() const {
    if (!is_array()) throw std::runtime_error("JSON value is not an array");
    return std::get<Array>(data_);
}

Value::Object& Value::as_object() {
    if (!is_object()) throw std::runtime_error("JSON value is not an object");
    return std::get<Object>(data_);
}

Value::Array& Value::as_array() {
    if (!is_array()) throw std::runtime_error("JSON value is not an array");
    return std::get<Array>(data_);
}

std::string Value::dump(int indent) const {
    return dump_impl(indent, 0);
}

std::string Value::dump_impl(int indent, int level) const {
    std::ostringstream oss;
    auto newline = [&](bool trailing) {
        if (indent > 0) {
            oss << '\n';
            if (trailing) {
                oss << std::string(level * indent, ' ');
            }
        }
    };

    if (is_null()) {
        oss << "null";
    } else if (is_bool()) {
        oss << (as_bool() ? "true" : "false");
    } else if (is_number()) {
        oss << as_number();
    } else if (is_string()) {
        oss << escape_string(as_string());
    } else if (is_array()) {
        const auto& arr = std::get<Array>(data_);
        oss << '[';
        if (!arr.empty()) {
            newline(true);
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) {
                    oss << ',';
                    newline(true);
                }
                oss << arr[i].dump_impl(indent, level + 1);
            }
            newline(false);
        }
        oss << ']';
    } else if (is_object()) {
        const auto& obj = std::get<Object>(data_);
        oss << '{';
        if (!obj.empty()) {
            newline(true);
            size_t index = 0;
            for (const auto& [k, v] : obj) {
                if (index++ > 0) {
                    oss << ',';
                    newline(true);
                }
                oss << escape_string(k) << ':';
                if (indent > 0) oss << ' ';
                oss << v.dump_impl(indent, level + 1);
            }
            newline(false);
        }
        oss << '}';
    }
    return oss.str();
}

Value parse(const std::string& text) {
    Parser parser(text);
    Value root = parser.parse_value();
    parser.ensure_consumed();
    return root;
}

}  // namespace mini_json
