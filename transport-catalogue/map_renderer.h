#pragma once

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

#include "geo.h"
#include "svg.h"
#include "json.h"
#include "transport_catalogue.h"

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

struct RenderSettings {
    double width = 0.0; //ширина изображения в пикселях
    double height = 0.0; //высота изображения в пикселях

    double padding = 0.0; //отступ краёв карты от границ SVG - документа

    double line_width = 0.0; //толщина линий, которыми рисуются автобусные маршруты
    double stop_radius = 0.0; //радиус окружностей, которыми обозначаются остановки

    int bus_label_font_size = 0; //размер текста, которым написаны названия автобусных маршрутов
    //смещение надписи с названием маршрута относительно координат конечной остановки на карте
    //Задаёт значения свойств dx и dy SVG-элемента <text>
    svg::Point bus_label_offset = { 0.0, 0.0 }; 

    int stop_label_font_size = 0; //размер текста, которым отображаются названия остановок
    //смещение названия остановки относительно её координат на карте
    //Задаёт значения свойств dx и dy SVG-элемента <text>
    svg::Point stop_label_offset = { 0.0, 0.0 };

    svg::Color underlayer_color; //цвет подложки под названиями остановок и маршрутов
    //толщина подложки под названиями остановок и маршрутов
    //Задаёт значение атрибута stroke-width элемента <text>
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette;
};

inline svg::Color GetColor(const json::Array* colors) {
    int8_t size = colors->size();
    if (size == int8_t(3)) {
        uint8_t r = static_cast<uint8_t>(colors->at(0).AsInt()),
            g = static_cast<uint8_t>(colors->at(1).AsInt()),
            b = static_cast<uint8_t>(colors->at(2).AsInt());
        return svg::Rgb( r, g, b );
    }
    else {
        if (size == int8_t(4)) {
            uint8_t r = static_cast<uint8_t>(colors->at(0).AsInt()),
                g = static_cast<uint8_t>(colors->at(1).AsInt()),
                b = static_cast<uint8_t>(colors->at(2).AsInt());
            double op = colors->at(3).AsPureDouble();
            return svg::Rgba(r, g, b, op);
        }
    }
    throw std::invalid_argument("Invalid color");
}

inline RenderSettings SetRenderSettings(const json::Dict& dict) {
    using namespace std::literals;
    RenderSettings sets;

    sets.width = dict.at("width"s).AsDouble();
    sets.height = dict.at("height"s).AsDouble();
    sets.padding = dict.at("padding"s).AsDouble();
    sets.line_width = dict.at("line_width"s).AsDouble();
    sets.stop_radius = dict.at("stop_radius"s).AsDouble();
    sets.bus_label_font_size = dict.at("bus_label_font_size"s).AsInt();
    {
        const json::Array* bus_offsets = &dict.at("bus_label_offset"s).AsArray();
        sets.bus_label_offset = { bus_offsets->at(0).AsDouble(), bus_offsets->at(1).AsDouble() };
    }
    sets.stop_label_font_size = dict.at("stop_label_font_size"s).AsInt();
    {
        const json::Array* stop_offsets = &dict.at("stop_label_offset"s).AsArray();
        sets.stop_label_offset = { stop_offsets->at(0).AsDouble(), stop_offsets->at(1).AsDouble() };
    }
    {
        const json::Node* underlayer_color = &dict.at("underlayer_color"s);
        if (underlayer_color->IsString()) {
            sets.underlayer_color = underlayer_color->AsString();
        }
        else {
            sets.underlayer_color = GetColor(&underlayer_color->AsArray());
        }
    }
    sets.underlayer_width = dict.at("underlayer_width"s).AsDouble();
    {
        const json::Array* palette = &dict.at("color_palette"s).AsArray();
        for (const json::Node& color : *palette) {
            if (color.IsString()) {
                sets.color_palette.push_back(color.AsString());
            }
            else {
                sets.color_palette.push_back(GetColor(&color.AsArray()));
            }
        }
    }

    return sets;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};




inline void SetBusUnderlayerSettings(svg::Text* bus_label_underlayer, const transport_catalogue::Bus* bus, int stop_num,
                                                const RenderSettings* sets, const SphereProjector& proj) {
    using namespace std::literals;
    bus_label_underlayer->SetData(bus->route_name)
        .SetPosition(proj(bus->route.at(stop_num)->coordinates))
        .SetOffset(sets->bus_label_offset)
        .SetFontSize(sets->bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetFillColor(sets->underlayer_color)
        .SetStrokeColor(sets->underlayer_color)
        .SetStrokeWidth(sets->underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

inline void SetBusLabelSettings(svg::Text* bus_label, const transport_catalogue::Bus* bus, int stop_num,
                                                const RenderSettings* sets, const SphereProjector& proj, int palette_count) {
    using namespace std::literals;
    bus_label->SetData(bus->route_name)
        .SetPosition(proj(bus->route.at(stop_num)->coordinates))
        .SetOffset(sets->bus_label_offset)
        .SetFontSize(sets->bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetFillColor(sets->color_palette[palette_count]);
}

inline void SetStopUnderlayerSettings(svg::Text* stop_label_underlayer, const transport_catalogue::Stop* stop,
                                                const RenderSettings* sets, const SphereProjector& proj) {
    using namespace std::literals;
    stop_label_underlayer->SetData(stop->name)
        .SetPosition(proj(stop->coordinates))
        .SetOffset(sets->stop_label_offset)
        .SetFontSize(sets->stop_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFillColor(sets->underlayer_color)
        .SetStrokeColor(sets->underlayer_color)
        .SetStrokeWidth(sets->underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

inline void SetStopLabelSettings(svg::Text* stop_label, const transport_catalogue::Stop* stop,
                                                const RenderSettings* sets, const SphereProjector& proj) {
    using namespace std::literals;
    stop_label->SetData(stop->name)
        .SetPosition(proj(stop->coordinates))
        .SetOffset(sets->stop_label_offset)
        .SetFontSize(sets->stop_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFillColor("black"s);
}

inline void AddRouteLines(svg::Document* svg_doc, const RenderSettings* render_settings,
                    const std::vector<const transport_catalogue::Bus*>* sorted_buses,
                    const SphereProjector& proj, int palette_count, int colours_num) {
    using namespace std::literals;
    for (const transport_catalogue::Bus* bus : *sorted_buses) {
        if (!bus->route.empty()) {
            svg::Polyline route_line;
            route_line.SetFillColor("none"s)
                .SetStrokeColor(render_settings->color_palette[palette_count])
                .SetStrokeWidth(render_settings->line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            ++palette_count;
            if (palette_count == colours_num) palette_count = 0;

            for (const transport_catalogue::Stop* stop : bus->route) {
                route_line.AddPoint(proj(stop->coordinates));
            }
            svg_doc->Add(std::move(route_line));
        }
    }
}

inline void AddBusesLabels(svg::Document* svg_doc, const RenderSettings* render_settings,
                    const std::vector<const transport_catalogue::Bus*>* sorted_buses,
                    const SphereProjector& proj, int palette_count, int colours_num) {
    for (const transport_catalogue::Bus* bus : *sorted_buses) {
        if (!bus->route.empty()) {
            svg::Text bus_label_underlayer;
            SetBusUnderlayerSettings(&bus_label_underlayer, bus, 0, render_settings, proj);
            svg_doc->Add(std::move(bus_label_underlayer));

            svg::Text bus_label;
            SetBusLabelSettings(&bus_label, bus, 0, render_settings, proj, palette_count);
            svg_doc->Add(std::move(bus_label));

            if (int second_stop_index = bus->route.size() / 2; !bus->is_roundtrip && bus->route.at(second_stop_index)->name != bus->route.at(0)->name) {
                svg::Text bus_second_label_underlayer;
                SetBusUnderlayerSettings(&bus_second_label_underlayer, bus, second_stop_index, render_settings, proj);
                svg_doc->Add(std::move(bus_second_label_underlayer));

                svg::Text bus_second_label;
                SetBusLabelSettings(&bus_second_label, bus, second_stop_index, render_settings, proj, palette_count);
                svg_doc->Add(std::move(bus_second_label));

            }
            ++palette_count;
            if (palette_count == colours_num) palette_count = 0;
        }
    }
}

inline void AddStopsSymbols(svg::Document* svg_doc, const RenderSettings* render_settings,
                    const std::vector<const transport_catalogue::Stop*>* all_sorted_stops,
                    const SphereProjector& proj) {
    using namespace std::literals;

    for (const transport_catalogue::Stop* stop : *all_sorted_stops) {
        svg::Circle stop_symbol;
        stop_symbol.SetCenter(proj(stop->coordinates))
            .SetRadius(render_settings->stop_radius)
            .SetFillColor("white"s);

        svg_doc->Add(stop_symbol);
    }

}

inline void AddStopLabels(svg::Document* svg_doc, const RenderSettings* render_settings,
                    const std::vector<const transport_catalogue::Stop*>* all_sorted_stops,
                    const SphereProjector& proj) {
    for (const transport_catalogue::Stop* stop : *all_sorted_stops) {
        svg::Text stop_label_underlayer;
        SetStopUnderlayerSettings(&stop_label_underlayer, stop, render_settings, proj);

        svg_doc->Add(std::move(stop_label_underlayer));

        svg::Text stop_label;
        SetStopLabelSettings(&stop_label, stop, render_settings, proj);

        svg_doc->Add(std::move(stop_label));
    }
}

inline void CreateMap(const json::Document* doc_, transport_catalogue::TransportCatalogue* catalogue_, std::ostream& output) {
    using namespace std::literals;
    svg::Document svg_doc;
    //GetRoot().AsMap().at("render_settings"s).AsMap()
    RenderSettings render_settings = SetRenderSettings(doc_->GetRoot().AsMap().at("render_settings"s).AsMap());

    const std::vector<const transport_catalogue::Bus*> sorted_buses = catalogue_->GetBusCatalogue();

    std::vector<geo::Coordinates> all_stops_coords;
    for (const transport_catalogue::Bus* bus : sorted_buses) {
        for (const transport_catalogue::Stop* stop : bus->route) {
            all_stops_coords.push_back(stop->coordinates);
        }
    }
    const SphereProjector proj{
        all_stops_coords.begin(), all_stops_coords.end(), render_settings.width, render_settings.height, render_settings.padding };

    int palette_count = 0;
    int colours_num = render_settings.color_palette.size();

    AddRouteLines(&svg_doc, &render_settings, &sorted_buses, proj, palette_count, colours_num);

    palette_count = 0;

    AddBusesLabels(&svg_doc, &render_settings, &sorted_buses, proj, palette_count, colours_num);

    ////////////////////////////////////////////////////////////////////////////// символы остановок
    std::vector<const transport_catalogue::Stop*> all_sorted_stops = catalogue_->GetStopCatalogue();

    AddStopsSymbols(&svg_doc, &render_settings, &all_sorted_stops, proj);

    ////////////////////////////////////////////////////////////////////////////// названия остановок
    AddStopLabels(&svg_doc, &render_settings, &all_sorted_stops, proj);

    svg_doc.Render(output);
    
}