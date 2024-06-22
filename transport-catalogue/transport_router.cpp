#pragma once

#include "transport_router.h"
#include "graph.h"

const double SPEED_COEF = 1000.0 / 60; //from km/hour to m/min

void TransportRouter::BuildGraph(const transport_catalogue::TransportCatalogue& catalogue) {
    auto sorted_stops = catalogue.GetSortedStops();
    auto sorted_buses = catalogue.GetSortedBuses();
    graph::VertexId vertex_id = 0;
    graph_ = graph::DirectedWeightedGraph<double>(sorted_stops.size() * 2);
    for (const auto& [stop_name, stop_info] : sorted_stops) {
        stop_ids_.emplace(std::make_pair(stop_name, vertex_id));
        graph_.AddEdge({
                                    stop_info->name,
                                    0,
                                    vertex_id,
                                    ++vertex_id,
                                    bus_wait_time_
            });
        ++vertex_id;
    }
    {
        for (const auto& [bus_name, bus] : sorted_buses) {
            const auto& bus_stops = bus->route;
            size_t stops_count = bus_stops.size();
            if (bus->is_roundtrip) {
                for (size_t i = 0; i < stops_count; ++i) {
                    for (size_t j = i + 1; j < stops_count; ++j) {
                        const transport_catalogue::Stop* stop_from = bus_stops[i];
                        const transport_catalogue::Stop* stop_to = bus_stops[j];
                        if (i == 0 && j == stops_count - 1) {
                            continue;
                        }
                        double dist_sum = 0.0;

                        for (size_t k = i + 1; k <= j; ++k) {
                            dist_sum += catalogue.GetDistance(bus_stops[k - 1]->name, bus_stops[k]->name);
                        }
                        graph_.AddEdge({
                                                    bus->route_name,
                                                    j - i,
                                                    stop_ids_.at(stop_from->name) + 1,
                                                    stop_ids_.at(stop_to->name),
                                                    dist_sum / (bus_velocity_ * SPEED_COEF)
                            });
                    }
                }
            }
            else {
                size_t half_stops_count = stops_count / 2;
                for (size_t i = 0; i < half_stops_count; ++i) {
                    for (size_t j = i + 1; j <= half_stops_count; ++j) {
                        const transport_catalogue::Stop* stop_from = bus_stops[i];
                        const transport_catalogue::Stop* stop_to = bus_stops[j];
                        double dist_sum = 0.0;

                        for (size_t k = i + 1; k <= j; ++k) {
                            dist_sum += catalogue.GetDistance(bus_stops[k - 1]->name, bus_stops[k]->name);
                        }
                        graph_.AddEdge({
                                                    bus->route_name,
                                                    j - i,
                                                    stop_ids_.at(stop_from->name) + 1,
                                                    stop_ids_.at(stop_to->name),
                                                    dist_sum / (bus_velocity_ * SPEED_COEF)
                            });
                    }
                }

                for (size_t i = half_stops_count; i < stops_count; ++i) {
                    for (size_t j = i + 1; j < stops_count; ++j) {
                        const transport_catalogue::Stop* stop_from = bus_stops[i];
                        const transport_catalogue::Stop* stop_to = bus_stops[j];
                        double dist_sum = 0.0;

                        for (size_t k = i + 1; k <= j; ++k) {
                            dist_sum += catalogue.GetDistance(bus_stops[k - 1]->name, bus_stops[k]->name);
                        }
                        graph_.AddEdge({
                                                    bus->route_name,
                                                    j - i,
                                                    stop_ids_.at(stop_from->name) + 1,
                                                    stop_ids_.at(stop_to->name),
                                                    dist_sum / (bus_velocity_ * SPEED_COEF)
                            });
                    }
                }

            }
        }
    }
    router_ = std::make_unique<graph::Router<double>>(graph_);
}

const std::optional<graph::Router<double>::RouteInfo> TransportRouter::FindRoute(const std::string_view from, const std::string_view to) const {
    if (!stop_ids_.count(std::string(from)) || !stop_ids_.count(std::string(to))) {
        return std::nullopt;
    }
    return router_->BuildRoute(stop_ids_.at(std::string(from)), stop_ids_.at(std::string(to)));
}

const graph::DirectedWeightedGraph<double>& TransportRouter::GetGraph() const {
    return graph_;
}