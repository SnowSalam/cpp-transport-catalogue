#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {
    using namespace std::literals;

    class Node;
    // —охраните объ€влени€ Dict и Array без изменени€
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;
    using Value = std::variant<std::nullptr_t, bool, int, double, std::string, Array, Dict>;

    // Ёта ошибка должна выбрасыватьс€ при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    std::ostream& operator<<(std::ostream& os, const Node& nod);

    class Node {
    public:
        Node() = default;
        Node(std::nullptr_t val) : value_(val) {}
        Node(int val) : value_(val) {}
        Node(double val) : value_(val) {}
        Node(bool val) : value_(val) {}
        Node(std::string val) : value_(std::move(val)) {}
        Node(Array val) : value_(std::move(val)) {}
        Node(Dict val) : value_(std::move(val)) {}


        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        ////////////////////// ASs

        int AsInt() const {
            if (!IsInt()) {
                throw std::logic_error("non-valid type");
            }
            return *std::get_if<int>(&value_);
        }
        double AsDouble() const {
            if (!IsDouble()) {
                throw std::logic_error("non-valid type");
            }
            if (std::get_if<double>(&value_)) {
                return*(std::get_if<double>(&value_));
            }
            return static_cast<double>(*std::get_if<int>(&value_));
        }
        double AsPureDouble() const {
            if (!IsPureDouble()) {
                throw std::logic_error("non-valid type");
            }
            return *std::get_if<double>(&value_);
        }
        bool AsBool() const {
            if (!IsBool()) {
                throw std::logic_error("non-valid type");
            }
            return *std::get_if<bool>(&value_);
        }
        const std::string& AsString() const {
            if (!IsString()) {
                throw std::logic_error("non-valid type");
            }
            return *std::get_if<std::string>(&value_);
        }
        const Array& AsArray() const {
            if (!IsArray()) {
                throw std::logic_error("non-valid type");
            }
            return *std::get_if<Array>(&value_);
        }
        const Dict& AsMap() const {
            if (!IsMap()) {
                throw std::logic_error("non-valid type");
            }
            return *std::get_if<Dict>(&value_);
        }

        const Value& GetValue() const {
            return value_;
        }

        bool operator==(const Node& other) const {
            return other.value_ == this->value_;
        }
        bool operator!=(const Node& other) const {
            return !(*this == other);
        }

    private:
        Value value_;
    };

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

        bool operator==(const Document& other) {
            return this->GetRoot() == other.GetRoot();
        }
        bool operator!=(const Document& other) {
            return !(*this == other);
        }

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

    struct NodeVisiter {
        std::ostream& out;

        void operator()(std::nullptr_t) {
            out << "null";
        }
        void operator()(bool v) {
            out << std::boolalpha << v;
        }
        void operator()(double v) {
            out << v;
        }
        void operator()(int v) {
            out << v;
        }
        void operator()(const std::string& v) {
            out << "\""sv;
            for (char c : v) {
                switch (c) {
                case'\\':
                    out << "\\\\"sv;
                    break;
                case'"':
                    out << "\\\""sv;
                    break;
                case'\n':
                    out << "\\n"sv;
                    break;
                case'\r':
                    out << "\\r"sv;
                    break;
                case'\t':
                    out << "\\t"sv;
                    break;
                default: out << c;
                    break;
                }
            }
            out << "\""sv;
        }
        void operator()(const Array& v) {
            out << "["sv;
            for (size_t i = 0; i < v.size(); ++i) {
                out << v[i];
                if (i + 1 == v.size()) { break; }
                out << ",";
            }
            out << "]"sv;
        }
        void operator()(const Dict& v) {

            size_t lim = v.size();
            out << "{"sv;
            for (const auto& i : v) {
                out << '"' << i.first << "\":" << i.second;
                --lim; if (lim == 0)break;
                out << ",";

            }
            out << "}"sv;
        }
    };

}  // namespace json