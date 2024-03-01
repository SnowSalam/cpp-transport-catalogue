#include "map_renderer.h"

using namespace std::literals;

void MapRenderer::SetBusUnderlayerSettings(svg::Text* bus_label_underlayer, const transport_catalogue::Bus* bus, int stop_num) {
    bus_label_underlayer->SetData(bus->route_name)
        .SetPosition(proj_(bus->route.at(stop_num)->coordinates))
        .SetOffset(render_settings_.bus_label_offset)
        .SetFontSize(render_settings_.bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetFillColor(render_settings_.underlayer_color)
        .SetStrokeColor(render_settings_.underlayer_color)
        .SetStrokeWidth(render_settings_.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

void MapRenderer::SetBusLabelSettings(svg::Text* bus_label, const transport_catalogue::Bus* bus, int stop_num, int palette_count) {
    bus_label->SetData(bus->route_name)
        .SetPosition(proj_(bus->route.at(stop_num)->coordinates))
        .SetOffset(render_settings_.bus_label_offset)
        .SetFontSize(render_settings_.bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetFillColor(render_settings_.color_palette[palette_count]);
}

void MapRenderer::SetStopUnderlayerSettings(svg::Text* stop_label_underlayer, const transport_catalogue::Stop* stop) {
    stop_label_underlayer->SetData(stop->name)
        .SetPosition(proj_(stop->coordinates))
        .SetOffset(render_settings_.stop_label_offset)
        .SetFontSize(render_settings_.stop_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFillColor(render_settings_.underlayer_color)
        .SetStrokeColor(render_settings_.underlayer_color)
        .SetStrokeWidth(render_settings_.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

void MapRenderer::SetStopLabelSettings(svg::Text * stop_label, const transport_catalogue::Stop * stop) {
    stop_label->SetData(stop->name)
        .SetPosition(proj_(stop->coordinates))
        .SetOffset(render_settings_.stop_label_offset)
        .SetFontSize(render_settings_.stop_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFillColor("black"s);
}

svg::Color GetColor(const json::Array* colors) {
    int8_t size = colors->size();
    if (size == int8_t(3)) {
        uint8_t r = static_cast<uint8_t>(colors->at(0).AsInt()),
            g = static_cast<uint8_t>(colors->at(1).AsInt()),
            b = static_cast<uint8_t>(colors->at(2).AsInt());
        return svg::Rgb(r, g, b);
    }
    else {
        if (size == int8_t(4)) {
            uint8_t r = static_cast<uint8_t>(colors->at(0).AsInt()),
                g = static_cast<uint8_t>(colors->at(1).AsInt()),
                b = static_cast<uint8_t>(colors->at(2).AsInt());
            double op = colors->at(3).AsDouble();
            return svg::Rgba(r, g, b, op);
        }
    }
    throw std::invalid_argument("Invalid color");
}

RenderSettings MapRenderer::SetRenderSettings(const json::Dict& dict) {
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

void MapRenderer::AddRouteLines(const std::vector<const transport_catalogue::Bus*>* sorted_buses, int palette_count, int colours_num) {
    for (const transport_catalogue::Bus* bus : *sorted_buses) {
        if (!bus->route.empty()) {
            svg::Polyline route_line;
            route_line.SetFillColor("none"s)
                .SetStrokeColor(render_settings_.color_palette[palette_count])
                .SetStrokeWidth(render_settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            ++palette_count;
            if (palette_count == colours_num) palette_count = 0;

            for (const transport_catalogue::Stop* stop : bus->route) {
                route_line.AddPoint(proj_(stop->coordinates));
            }
            svg_doc_.Add(std::move(route_line));
        }
    }
}

void MapRenderer::AddBusesLabels(const std::vector<const transport_catalogue::Bus*>* sorted_buses, int palette_count, int colours_num) {
    for (const transport_catalogue::Bus* bus : *sorted_buses) {
        if (!bus->route.empty()) {
            svg::Text bus_label_underlayer;
            SetBusUnderlayerSettings(&bus_label_underlayer, bus, 0);
            svg_doc_.Add(std::move(bus_label_underlayer));

            svg::Text bus_label;
            SetBusLabelSettings(&bus_label, bus, 0, palette_count);
            svg_doc_.Add(std::move(bus_label));

            if (int second_stop_index = bus->route.size() / 2; !bus->is_roundtrip && bus->route.at(second_stop_index)->name != bus->route.at(0)->name) {
                svg::Text bus_second_label_underlayer;
                SetBusUnderlayerSettings(&bus_second_label_underlayer, bus, second_stop_index);
                svg_doc_.Add(std::move(bus_second_label_underlayer));

                svg::Text bus_second_label;
                SetBusLabelSettings(&bus_second_label, bus, second_stop_index, palette_count);
                svg_doc_.Add(std::move(bus_second_label));

            }
            ++palette_count;
            if (palette_count == colours_num) palette_count = 0;
        }
    }
}

void MapRenderer::AddStopsSymbols(const std::vector<const transport_catalogue::Stop*>* all_sorted_stops) {

    for (const transport_catalogue::Stop* stop : *all_sorted_stops) {
        svg::Circle stop_symbol;
        stop_symbol.SetCenter(proj_(stop->coordinates))
            .SetRadius(render_settings_.stop_radius)
            .SetFillColor("white"s);

        svg_doc_.Add(stop_symbol);
    }
}

void MapRenderer::AddStopLabels(const std::vector<const transport_catalogue::Stop*>* all_sorted_stops) {
    for (const transport_catalogue::Stop* stop : *all_sorted_stops) {
        svg::Text stop_label_underlayer;
        SetStopUnderlayerSettings(&stop_label_underlayer, stop);

        svg_doc_.Add(std::move(stop_label_underlayer));

        svg::Text stop_label;
        SetStopLabelSettings(&stop_label, stop);

        svg_doc_.Add(std::move(stop_label));
    }
}

MapRenderer& MapRenderer::CreateMap() {
    const std::vector<const transport_catalogue::Bus*> sorted_buses = catalogue_->GetBusCatalogue();

    {
        std::vector<geo::Coordinates> all_stops_coords;
        for (const transport_catalogue::Bus* bus : sorted_buses) {
            for (const transport_catalogue::Stop* stop : bus->route) {
                all_stops_coords.push_back(stop->coordinates);
            }
        }

        proj_ = SphereProjector{
        all_stops_coords.begin(), all_stops_coords.end(), render_settings_.width, render_settings_.height, render_settings_.padding };
    }

    int palette_count = 0;
    int colours_num = render_settings_.color_palette.size();

    // routes' lines
    AddRouteLines(&sorted_buses, palette_count, colours_num);

    palette_count = 0;

    AddBusesLabels(&sorted_buses, palette_count, colours_num);

    // stops' symbols
    std::vector<const transport_catalogue::Stop*> all_sorted_stops = catalogue_->GetStopCatalogue();

    AddStopsSymbols(&all_sorted_stops);

    // stops' labels
    AddStopLabels(&all_sorted_stops);

    return *this;
}

void MapRenderer::RenderMap(std::ostream& output) {
    svg_doc_.Render(output);
}