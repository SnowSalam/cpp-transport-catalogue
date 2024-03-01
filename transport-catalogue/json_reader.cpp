#include "json_reader.h"
#include "json_builder.h"

// сортировка запросов на добавление в каталог
void JsonReader::SortInputRequests(std::vector<json::Node>& buses, std::vector<json::Node>& stops) {
    using namespace std::literals;

    for (const json::Node& node : doc_.GetRoot().AsDict().at("base_requests"s).AsArray()) {
        if (node.AsDict().at("type"s).AsString() == "Bus"s) {
            buses.emplace_back(std::move(node));
        }
        else {
            stops.emplace_back(std::move(node));
        }
    }
}

// добавление остановок и дистанций
void JsonReader::AddStops(std::vector<json::Node>* stops) {
    using namespace std::literals;

    // добавление остановок
    for (json::Node& node : *stops) {
        geo::Coordinates coordinates{ node.AsDict().at("latitude"s).AsDouble(), node.AsDict().at("longitude"s).AsDouble() };
        catalogue_.AddStop(node.AsDict().at("name"s).AsString(), coordinates);
    }
    // добавление дистанций
    for (json::Node& node : *stops) {
        std::string_view stop_from = node.AsDict().at("name"s).AsString();
        for (const std::pair<const std::string, json::Node>& stop_to : node.AsDict().at("road_distances"s).AsDict()) {
            catalogue_.AddDistances(stop_from, stop_to);
        }
    }
}

// добавление автобусов
void JsonReader::AddBuses(std::vector<json::Node>* buses) {
    using namespace std::literals;

    for (json::Node& node : *buses) {
        bool is_round = node.AsDict().at("is_roundtrip"s).AsBool();
        std::vector<std::string_view> stops;
        for (const json::Node& stop_node : node.AsDict().at("stops"s).AsArray()) {
            stops.push_back(stop_node.AsString());
        }
        if (!is_round) {
            std::vector<std::string_view> stops_copy(stops.begin(), stops.end());
            stops.insert(stops.end(), std::next(stops_copy.rbegin()), stops_copy.rend());
        }

        catalogue_.AddBus(node.AsDict().at("name"s).AsString(), move(stops), is_round);
    }
}

void JsonReader::FillCatalogue() {
    using namespace std;

    std::vector<json::Node> buses_input;
    std::vector<json::Node> stops_input;

    SortInputRequests(buses_input, stops_input);

    AddStops(&stops_input);

    AddBuses(&buses_input);

}

json::Document FormResponce(const json::Array* stat_requests, const transport_catalogue::TransportCatalogue* catalogue, const json::Document* doc) {
    using namespace std::literals;

    json::Builder builder{}; // выдаёт Node

    builder.StartArray();

    for (const json::Node& node : *stat_requests) {

        int id = node.AsDict().at("id"s).AsInt();

        if (node.AsDict().at("type"s).AsString() == "Bus"s) {
            if (std::optional<transport_catalogue::BusInfo> bus = catalogue->GetBusInfo(node.AsDict().at("name"s).AsString()); bus != std::nullopt) {
                builder.StartDict()
                    .Key("curvature"s).Value(bus->curvature)
                    .Key("request_id"s).Value(id)
                    .Key("route_length"s).Value(bus->route_length)
                    .Key("stop_count"s).Value(bus->stops_count)
                    .Key("unique_stop_count"s).Value(bus->unique_stops_count)
                    .EndDict();
            }
            else {
                builder.StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                    .EndDict();
            }
        }
        else {
            if (node.AsDict().at("type"s).AsString() == "Stop"s) {
                if (std::optional<std::set<std::string_view>> buses_of_stop = catalogue->GetStopInfo(node.AsDict().at("name"s).AsString()); buses_of_stop != std::nullopt) {

                    builder.StartDict().Key("buses"s).StartArray();

                    for (const std::string_view& bus : *buses_of_stop) {
                        builder.Value(static_cast<std::string>(bus));
                    }
                    builder.EndArray()
                        .Key("request_id"s).Value(id)
                        .EndDict();
                }
                else {
                    builder.StartDict()
                        .Key("request_id"s).Value(id)
                        .Key("error_message"s).Value("not found"s)
                        .EndDict();
                }
            }
            else {
                if (node.AsDict().at("type"s).AsString() == "Map"s) {

                    std::ostringstream outstream;

                    MapRenderer map_renderer(doc, catalogue);
                    map_renderer.CreateMap().RenderMap(outstream);

                    builder.StartDict()
                        .Key("map"s).Value(outstream.str())
                        .Key("request_id"s).Value(id)
                        .EndDict();
                }
                else {
                    throw std::invalid_argument("Invalid request type"s);
                }
            }
        }
    }
    return json::Document(builder.EndArray().Build());
}

void JsonReader::PrintRequests(std::ostream& output) {
    using namespace std::literals;

    const json::Array* stat_requests = &doc_.GetRoot().AsDict().at("stat_requests"s).AsArray();
    json::Document responce = FormResponce(stat_requests, &catalogue_, &doc_);
    json::Print(responce, output);

}