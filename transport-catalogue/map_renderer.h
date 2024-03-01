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
    double width = 0.0; //������ ����������� � ��������
    double height = 0.0; //������ ����������� � ��������

    double padding = 0.0; //������ ���� ����� �� ������ SVG - ���������

    double line_width = 0.0; //������� �����, �������� �������� ���������� ��������
    double stop_radius = 0.0; //������ �����������, �������� ������������ ���������

    int bus_label_font_size = 0; //������ ������, ������� �������� �������� ���������� ���������
    //�������� ������� � ��������� �������� ������������ ��������� �������� ��������� �� �����
    //����� �������� ������� dx � dy SVG-�������� <text>
    svg::Point bus_label_offset = { 0.0, 0.0 }; 

    int stop_label_font_size = 0; //������ ������, ������� ������������ �������� ���������
    //�������� �������� ��������� ������������ � ��������� �� �����
    //����� �������� ������� dx � dy SVG-�������� <text>
    svg::Point stop_label_offset = { 0.0, 0.0 };

    svg::Color underlayer_color; //���� �������� ��� ���������� ��������� � ���������
    //������� �������� ��� ���������� ��������� � ���������
    //����� �������� �������� stroke-width �������� <text>
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette;
};

class SphereProjector {
public:
    SphereProjector() = default;
    // points_begin � points_end ������ ������ � ����� ��������� ��������� geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // ���� ����� ����������� ����� �� ������, ��������� ������
        if (points_begin == points_end) {
            return;
        }

        // ������� ����� � ����������� � ������������ ��������
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // ������� ����� � ����������� � ������������ �������
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // ��������� ����������� ��������������� ����� ���������� x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // ��������� ����������� ��������������� ����� ���������� y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // ������������ ��������������� �� ������ � ������ ���������,
            // ���� ����������� �� ���
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            // ����������� ��������������� �� ������ ���������, ���������� ���
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            // ����������� ��������������� �� ������ ���������, ���������� ���
            zoom_coeff_ = *height_zoom;
        }
    }

    // ���������� ������ � ������� � ���������� ������ SVG-�����������
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_ = 0;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer() = default;
    MapRenderer(const json::Document* doc, const transport_catalogue::TransportCatalogue* catalogue)
        : doc_(doc), catalogue_(catalogue) {
        render_settings_ = SetRenderSettings(doc_->GetRoot().AsDict().at("render_settings").AsDict());
    }

    MapRenderer& CreateMap();

    void RenderMap(std::ostream& output);

private:
    const json::Document* doc_;
    const transport_catalogue::TransportCatalogue* catalogue_;
    svg::Document svg_doc_;
    RenderSettings render_settings_;
    SphereProjector proj_;

    void SetBusUnderlayerSettings(svg::Text* bus_label_underlayer, const transport_catalogue::Bus* bus, int stop_num);

    void SetBusLabelSettings(svg::Text* bus_label, const transport_catalogue::Bus* bus, int stop_num, int palette_count);

    void SetStopUnderlayerSettings(svg::Text* stop_label_underlayer, const transport_catalogue::Stop* stop);

    void SetStopLabelSettings(svg::Text* stop_label, const transport_catalogue::Stop* stop);

    RenderSettings SetRenderSettings(const json::Dict& dict);

    void AddRouteLines(const std::vector<const transport_catalogue::Bus*>* sorted_buses, int palette_count, int colours_num);

    void AddBusesLabels(const std::vector<const transport_catalogue::Bus*>* sorted_buses, int palette_count, int colours_num);

    void AddStopsSymbols(const std::vector<const transport_catalogue::Stop*>* all_sorted_stops);

    void AddStopLabels(const std::vector<const transport_catalogue::Stop*>* all_sorted_stops);

};