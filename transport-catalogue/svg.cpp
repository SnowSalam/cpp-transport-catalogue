#include "svg.h"

namespace svg {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << "\n";
        //context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        // Выводим атрибуты, унаследованные от PathProps
        RenderAttrs(context.out);
        out << "/>"sv;

    }

    // --------- Polyline -----------------

    Polyline& Polyline::AddPoint(Point point) {
        vertexes_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << R"(<polyline points=")";
        bool is_first = true;
        for (Point point : vertexes_) {
            if (is_first) {
                out << point.x << "," << point.y;
                is_first = false;
            }
            else {
                out << " " << point.x << "," << point.y;
            }
        }
        out << "\"";
        RenderAttrs(context.out);
        out << "/>"s;
    }

    // ----------- Text -------------------

    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = data;
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << R"(<text)";
        RenderAttrs(context.out);
        out << R"( x=")" << position_.x << R"(" y=")" << position_.y << R"(" dx=")"
            << offset_.x << R"(" dy=")" << offset_.y << R"(" font-size=")" << font_size_ << "\"";
        if (!font_family_.empty()) {
            out << R"( font-family=")" << font_family_ << "\"";
        }

        if (!font_weight_.empty()) {
            out << R"( font-weight=")" << font_weight_ << "\"";
        }

        out << ">";

        std::string data;
        for (char symbol : data_) {
            if (symbol == '\"') {
                data.append(R"(&quot;)");
                continue;
            }
            if (symbol == '\'') {
                data.append(R"(&apos;)");
                continue;
            }
            if (symbol == '<') {
                data.append(R"(&lt;)");
                continue;
            }
            if (symbol == '>') {
                data.append(R"(&gt;)");
                continue;
            }
            if (symbol == '&') {
                data.append(R"(&amp;)");
                continue;
            }
            data.append(1, symbol);
        }
        out << data << "</text>";
    }

    // ------ ObjectContainer -------------

    void ObjectContainer::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
        RenderContext context(out);
        for (auto& object : objects_) {
            out << "  ";
            object->Render(context);
        }
        out << "</svg>"s;
    }

    // --------- Document -----------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.emplace_back(std::move(obj));
    }

}  // namespace svg