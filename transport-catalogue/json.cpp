#include "json.h"

using namespace std;

namespace json {

    using Number = std::variant<int, double>;

    bool Node::IsInt() const {
        return std::holds_alternative<int>(value_);
    }
    bool Node::IsDouble() const {
        if (std::holds_alternative<double>(value_) || std::holds_alternative<int>(value_)) {
            return true;
        }
        else return 0;
    }
    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(value_);
    }
    bool Node::IsBool() const {
        return std::holds_alternative<bool>(value_);
    }
    bool Node::IsString() const {
        return std::holds_alternative<std::string>(value_);
    }
    bool Node::IsNull() const {
        return std::holds_alternative<nullptr_t>(value_);
    }
    bool Node::IsArray() const {
        return std::holds_alternative<Array>(value_);
    }
    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(value_);
    }

    Node LoadNode(istream& input);

    Node LoadNull(istream& input) {
        const string nameNull = "null";
        for (size_t i = 0; i < nameNull.size(); i++) {
            if (nameNull.at(i) == input.get()) continue;
            else throw ParsingError("Null parsing error");
        }
        if (char check = input.peek(); (check > 64 && check < 91) || (check > 96 && check < 123)) {
            throw ParsingError("Bool parsing error");
        }
        return {};
    }

    // ��������� ���������� ���������� �������� JSON-���������
    // ������� ������� ������������ ����� ���������� ������������ ������� ":
    std::string LoadString(std::istream& input) {
        using namespace std::literals;

        auto it = std::istreambuf_iterator<char>(input);
        auto end = std::istreambuf_iterator<char>();
        std::string s;
        while (true) {
            if (it == end) {
                // ����� ���������� �� ����, ��� ��������� ����������� �������?
                throw ParsingError("String parsing error");
            }
            const char ch = *it;
            if (ch == '"') {
                // ��������� ����������� �������
                ++it;
                break;
            }
            else if (ch == '\\') {
                // ��������� ������ escape-������������������
                ++it;
                if (it == end) {
                    // ����� ���������� ����� ����� ������� �������� ����� �����
                    throw ParsingError("String parsing error");
                }
                const char escaped_char = *(it);
                // ������������ ���� �� �������������������: \\, \n, \t, \r, \"
                switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // ��������� ����������� escape-������������������
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                }
            }
            else if (ch == '\n' || ch == '\r') {
                // ��������� ������� ������- JSON �� ����� ����������� ��������� \r ��� \n
                throw ParsingError("Unexpected end of line"s);
            }
            else {
                // ������ ��������� ��������� ������ � �������� ��� � �������������� ������
                s.push_back(ch);
            }
            ++it;
        }

        return s;
    }

    Node LoadNumber(std::istream& input) {
        using namespace std::literals;

        std::string parsed_num;

        // ��������� � parsed_num ��������� ������ �� input
        auto read_char = [&parsed_num, &input] {
            parsed_num += static_cast<char>(input.get());
            if (!input) {
                throw ParsingError("Failed to read number from stream"s);
            }
        };

        // ��������� ���� ��� ����� ���� � parsed_num �� input
        auto read_digits = [&input, read_char] {
            if (!std::isdigit(input.peek())) {
                throw ParsingError("A digit is expected"s);
            }
            while (std::isdigit(input.peek())) {
                read_char();
            }
        };

        if (input.peek() == '-') {
            read_char();
        }
        // ������ ����� ����� �����
        if (input.peek() == '0') {
            read_char();
            // ����� 0 � JSON �� ����� ���� ������ �����
        }
        else {
            read_digits();
        }

        bool is_int = true;
        // ������ ������� ����� �����
        if (input.peek() == '.') {
            read_char();
            read_digits();
            is_int = false;
        }

        // ������ ���������������� ����� �����
        if (int ch = input.peek(); ch == 'e' || ch == 'E') {
            read_char();
            if (ch = input.peek(); ch == '+' || ch == '-') {
                read_char();
            }
            read_digits();
            is_int = false;
        }

        try {
            if (is_int) {
                // ������� ������� ������������� ������ � int
                try {
                    return std::stoi(parsed_num);
                }
                catch (...) {
                    // � ������ �������, ��������, ��� ������������,
                    // ��� ���� ��������� ������������� ������ � double
                }
            }
            return std::stod(parsed_num);
        }
        catch (...) {
            throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
        }
    }

    Node LoadBool(istream& input) {
        const string nameFalse = "false";
        const string nameTrue = "true";
        char c = input.get();
        bool value = (c == 't');
        std::string const* name = value ? &nameTrue : &nameFalse;
        for (size_t i = 1; i < name->size(); i++) {
            if (name->at(i) == input.get()) continue;
            else throw ParsingError("Bool parsing error");
        }
        if (char check = input.peek(); (check > 64 && check < 91) || (check > 96 && check < 123)) {
            throw ParsingError("Bool parsing error");
        }
        return Node(value);
    }

    Node LoadArray(istream& input) {
        Array result;
        if (input.peek() == -1) throw ParsingError("Array parsing error");

        for (char c; input >> c && c != ']';) {
            if (c != ',') {
                input.putback(c);
            }
            result.push_back(LoadNode(input));
        }

        return Node(std::move(result));
    }

    Node LoadDict(istream& input) {
        Dict result;
        if (input.peek() == -1) throw ParsingError("Array parsing error");

        for (char c; input >> c && c != '}';) {
            if (c == ',') {
                input >> c;
            }

            string key = LoadString(input);
            input >> c;
            result.insert({ std::move(key), LoadNode(input) });
        }

        return Node(std::move(result));
    }

    Node LoadNode(istream& input) {
        char c;
        input >> c;

        if (c == 'n') {
            input.putback(c);
            return LoadNull(input);
        }
        else if (c == '"') {
            return LoadString(input);
        }
        else if (c == 't' || c == 'f') {
            input.putback(c);
            return LoadBool(input);
        }
        else if (c == '[') {
            return LoadArray(input);
        }
        else if (c == '{') {
            return LoadDict(input);
        }
        else {
            input.putback(c);
            return LoadNumber(input);
        }
        throw ParsingError("Empty request");
    }

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        std::visit(NodeVisiter{ output }, doc.GetRoot().GetValue());
    }

    std::ostream& operator<<(std::ostream& out, const Node& node) {
        std::visit(NodeVisiter{ out }, node.GetValue());
        return out;
    }

}  // namespace json