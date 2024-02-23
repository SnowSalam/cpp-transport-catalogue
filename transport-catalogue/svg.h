#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <iomanip>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "domain.h"

namespace svg {

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    // ������� � ������������ ����� ��������� �� �������������� inline,
// �� ������� ���, ��� ��� ����� ����� �� ��� ������� ����������,
// ������� ���������� ���� ���������.
// � ��������� ������ ������ ������� ���������� ����� ������������ ���� ����� ���� ���������
    inline const Color NoneColor{};

    struct ColorPrinter {
        std::ostream& out;

        void operator()(std::monostate) const {
            out << "none";
        }
        void operator()(std::string color) const {
            out << color;
        }
        void operator()(Rgb color) const {
            out << "rgb(" << std::to_string(color.red) << "," << std::to_string(color.green) << ","
                << std::to_string(color.blue) << ")";
        }
        void operator()(Rgba color) const {
            out << "rgba(" << std::to_string(color.red) << "," << std::to_string(color.green) << ","
                << std::to_string(color.blue) << "," << color.opacity << ")";
        }
    };

    inline std::ostream& operator<<(std::ostream& out, const Color color) {
        std::visit(ColorPrinter{ out }, color);
        return out;
    }

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    inline std::ostream& operator<<(std::ostream& out, const StrokeLineCap line_cap) {

        switch (line_cap)
        {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;
        case StrokeLineCap::ROUND:
            out << "round";
            break;
        case StrokeLineCap::SQUARE:
            out << "square";
            break;
        }
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, const StrokeLineJoin line_join) {

        switch (line_join)
        {
        case StrokeLineJoin::ARCS:
            out << "arcs";
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case StrokeLineJoin::MITER:
            out << "miter";
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case StrokeLineJoin::ROUND:
            out << "round";
            break;
        }
        return out;
    }

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0;
        double y = 0;
    };

    /*
     * ��������������� ���������, �������� �������� ��� ������ SVG-��������� � ���������.
     * ������ ������ �� ����� ������, ������� �������� � ��� ������� ��� ������ ��������
     */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * ����������� ������� ����� Object ������ ��� ���������������� ��������
     * ���������� ����� SVG-���������
     * ��������� ������� "��������� �����" ��� ������ ����������� ����
     */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeWidth(double width) {
            stroke_width_ = std::move(width);
            return AsOwner();
        }
        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_line_cap_ = line_cap;
            return AsOwner();
        }
        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_line_join_ = line_join;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        // ����� RenderAttrs ������� � ����� ����� ��� ���� ����� �������� fill � stroke
        void RenderAttrs(std::ostream& out) const {

            if (fill_color_) {
                out << " fill=\\\"" << *fill_color_ << "\\\"";
            }
            if (stroke_color_) {
                out << " stroke=\\\"" << *stroke_color_ << "\\\"";
            }
            if (stroke_width_) {
                out << R"( stroke-width=\")" << *stroke_width_ << "\\\"";
            }
            if (stroke_line_cap_) {
                out << R"( stroke-linecap=\")" << *stroke_line_cap_ << "\\\"";
            }
            if (stroke_line_join_) {
                out << R"( stroke-linejoin=\")" << *stroke_line_join_ << "\\\"";
            }
        }

    private:
        Owner& AsOwner() {
            // static_cast ��������� ����������� *this � Owner&,
            // ���� ����� Owner � ��������� PathProps
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
    };

    /*
     * ����� Circle ���������� ������� <circle> ��� ����������� �����
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
     */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle() = default;

        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_ = { 0, 0 };
        double radius_ = 1.0;
    };

    /*
     * ����� Polyline ���������� ������� <polyline> ��� ����������� ������� �����
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        Polyline() = default;

        // ��������� ��������� ������� � ������� �����
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> vertexes_;
    };

    /*
     * ����� Text ���������� ������� <text> ��� ����������� ������
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text final : public Object, public PathProps<Text> {
    public:
        Text() = default;

        // ����� ���������� ������� ����� (�������� x � y)
        Text& SetPosition(Point pos);

        // ����� �������� ������������ ������� ����� (�������� dx, dy)
        Text& SetOffset(Point offset);

        // ����� ������� ������ (������� font-size)
        Text& SetFontSize(uint32_t size);

        // ����� �������� ������ (������� font-family)
        Text& SetFontFamily(std::string font_family);

        // ����� ������� ������ (������� font-weight)
        Text& SetFontWeight(std::string font_weight);

        // ����� ��������� ���������� ������� (������������ ������ ���� text)
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point position_ = { 0, 0 };
        Point offset_ = { 0, 0 };
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;

    };

    class ObjectContainer {
    public:
        ObjectContainer() = default;

        /*
         ����� Add ��������� � svg-�������� ����� ������-��������� svg::Object.
         ������ �������������:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */
        template <typename Obj>
        void Add(Obj obj) {
            objects_.emplace_back(std::make_unique<Obj>(std::move(obj)));
        }

        // ��������� � svg-�������� ������-��������� svg::Object
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

        // ������� � ostream svg-������������� ���������
        void Render(std::ostream& out) const;

    protected:
        virtual ~ObjectContainer() = default;
        std::vector<std::unique_ptr<Object>> objects_;
    };

    class Document : public ObjectContainer {
    public:
        // ��������� � svg-�������� ������-��������� svg::Object
        void AddPtr(std::unique_ptr<Object>&& obj) override;
    };

    class Drawable {
    public:
        Drawable() = default;
        virtual ~Drawable() = default;

        virtual void Draw(ObjectContainer& container) const = 0;
    };

}  // namespace svg