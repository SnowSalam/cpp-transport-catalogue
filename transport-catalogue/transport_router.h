#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <algorithm>
#include <memory>
#include <iostream>

class TransportRouter {
public:
    TransportRouter() = default;

    TransportRouter& SetVelocity(double velocity) {
        bus_velocity_ = velocity;
        return *this;
    }
    TransportRouter& SetWaitTime(int time) {
        bus_wait_time_ = time;
        return *this;
    }

    void BuildGraph(const transport_catalogue::TransportCatalogue& catalogue);
    const std::optional<graph::Router<double>::RouteInfo> FindRoute(const std::string_view from, const std::string_view to) const;
    const graph::DirectedWeightedGraph<double>& GetGraph() const;
private:
    double bus_velocity_ = 0.0;
    double bus_wait_time_ = 0;

    graph::DirectedWeightedGraph<double> graph_;
    std::unordered_map<std::string_view, graph::VertexId> stop_ids_;
    std::unique_ptr<graph::Router<double>> router_;
};