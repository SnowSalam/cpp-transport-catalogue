#include "json_reader.h"
#include "json_builder.h"

using namespace std::literals;

// sorting requests for adding to the catalog
void JsonReader::SortInputRequests(std::vector<json::Node>& buses, std::vector<json::Node>& stops) {

    for (const json::Node& node : doc_.GetRoot().AsDict().at("base_requests"s).AsArray()) {
        if (node.AsDict().at("type"s).AsString() == "Bus"s) {
            buses.emplace_back(std::move(node));
        }
        else {
            stops.emplace_back(std::move(node));
        }
    }
}

// adding stops and distances
void JsonReader::AddStops(std::vector<json::Node>* stops) {

    // adding stops
    for (json::Node& node : *stops) {
        geo::Coordinates coordinates{ node.AsDict().at("latitude"s).AsDouble(), node.AsDict().at("longitude"s).AsDouble() };
        catalogue_.AddStop(node.AsDict().at("name"s).AsString(), coordinates);
    }
    // adding distances
    for (json::Node& node : *stops) {
        std::string_view stop_from = node.AsDict().at("name"s).AsString();
        for (const std::pair<const std::string, json::Node>& stop_to : node.AsDict().at("road_distances"s).AsDict()) {
            catalogue_.AddDistances(stop_from, stop_to);
        }
    }
}

// adding buses
void JsonReader::AddBuses(std::vector<json::Node>* buses) {

    for (json::Node& node : *buses) {
        bool is_round = node.AsDict().at("is_roundtrip"s).AsBool();
        std::vector<std::string_view> stops;
        for (const json::Node& stop_node : node.AsDict().at("stops"s).AsArray()) {
            stops.emplace_back(stop_node.AsString());
        }
        if (!is_round) {
            std::vector<std::string_view> stops_copy(stops.begin(), stops.end());
            stops.insert(stops.end(), std::next(stops_copy.rbegin()), stops_copy.rend());
        }

        catalogue_.AddBus(node.AsDict().at("name"s).AsString(), move(stops), is_round);
    }
}

void JsonReader::FillCatalogue() {

    std::vector<json::Node> buses_input;
    std::vector<json::Node> stops_input;

    SortInputRequests(buses_input, stops_input);

    AddStops(&stops_input);

    AddBuses(&buses_input);

}

void JsonReader::SetRouterSettings() {
    const json::Dict& router_sets_dict = doc_.GetRoot().AsDict().at("routing_settings"s).AsDict();
    router_.SetVelocity(router_sets_dict.at("bus_velocity"s).AsDouble())
        .SetWaitTime(router_sets_dict.at("bus_wait_time"s).AsInt());
}

void MakeErrorResponse(json::Builder& builder, int id) {
    builder.StartDict()
        .Key("request_id"s).Value(id)
        .Key("error_message"s).Value("not found"s)
        .EndDict();
}

json::Document JsonReader::FormResponce(const json::Array* stat_requests) {
    router_.BuildGraph(catalogue_);
    json::Builder builder{}; // gives Node

    builder.StartArray();

    for (const json::Node& node : *stat_requests) {

        int id = node.AsDict().at("id"s).AsInt();

        if (node.AsDict().at("type"s).AsString() == "Bus"s) {
            if (std::optional<transport_catalogue::BusInfo> bus = catalogue_.GetBusInfo(node.AsDict().at("name"s).AsString()); bus != std::nullopt) {
                builder.StartDict()
                    .Key("curvature"s).Value(bus->curvature)
                    .Key("request_id"s).Value(id)
                    .Key("route_length"s).Value(bus->route_length)
                    .Key("stop_count"s).Value(bus->stops_count)
                    .Key("unique_stop_count"s).Value(bus->unique_stops_count)
                    .EndDict();
            }
            else {
                MakeErrorResponse(builder, id);
            }
        }
        else if (node.AsDict().at("type"s).AsString() == "Stop"s) 
            {
                if (std::optional<std::set<std::string_view>> buses_of_stop = catalogue_.GetStopInfo(node.AsDict().at("name"s).AsString()); buses_of_stop != std::nullopt) {

                    builder.StartDict().Key("buses"s).StartArray();

                    for (const std::string_view& bus : *buses_of_stop) {
                        builder.Value(static_cast<std::string>(bus));
                    }
                    builder.EndArray()
                        .Key("request_id"s).Value(id)
                        .EndDict();
                }
                else {
                    MakeErrorResponse(builder, id);
                }
            }
            else if (node.AsDict().at("type"s).AsString() == "Map"s)
                {

                    std::ostringstream outstream;

                    MapRenderer map_renderer(&doc_, &catalogue_);
                    map_renderer.CreateMap().RenderMap(outstream);

                    builder.StartDict()
                        .Key("map"s).Value(outstream.str())
                        .Key("request_id"s).Value(id)
                        .EndDict();
                }
            else if (node.AsDict().at("type"s).AsString() == "Route"s)
        {
            const auto& routing = router_.FindRoute(node.AsDict().at("from").AsString(), node.AsDict().at("to").AsString());
            if (routing) {
                double total_time = 0.0;
                json::Array items;
                items.reserve(routing.value().edges.size());
                for (auto& edge_id : routing.value().edges) {
                    const graph::Edge<double> edge = router_.GetGraph().GetEdge(edge_id);
                    if (edge.span_count == 0) {
                        items.emplace_back(json::Node(json::Builder{}
                            .StartDict()
                            .Key("stop_name").Value(edge.name)
                            .Key("time").Value(edge.weight)
                            .Key("type").Value("Wait")
                            .EndDict()
                            .Build().AsDict()
                        )
                        );
                        total_time += edge.weight;
                    }
                    else {
                        items.emplace_back(json::Node(json::Builder{}
                            .StartDict()
                            .Key("bus").Value(edge.name)
                            .Key("span_count").Value(static_cast<int>(edge.span_count))
                            .Key("time").Value(edge.weight)
                            .Key("type").Value("Bus")
                            .EndDict()
                            .Build().AsDict()
                        )
                        );
                        total_time += edge.weight;
                    }
                }

                builder.StartDict()
                    .Key("items")
                    .Value(items)
                    .Key("total_time").Value(total_time)
                    .Key("request_id").Value(id)
                    .EndDict();
            }
            else {
                MakeErrorResponse(builder, id);
            }

        }
                else {
                    throw std::invalid_argument("Invalid request type"s);
                }
    }
    return json::Document(builder.EndArray().Build());
}

void JsonReader::PrintRequests(std::ostream& output) {

    const json::Array* stat_requests = &doc_.GetRoot().AsDict().at("stat_requests"s).AsArray();
    json::Document responce = FormResponce(stat_requests);
    json::Print(responce, output);

}