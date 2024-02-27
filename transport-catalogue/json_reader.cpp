#include "json_reader.h"

// сортировка запросов на добавление в каталог
void JsonReader::SortInputRequests(std::vector<json::Node>& buses, std::vector<json::Node>& stops) {
    using namespace std::literals;

    for (const json::Node& node : doc_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
        if (node.AsMap().at("type"s).AsString() == "Bus"s) {
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
        geo::Coordinates coordinates{ node.AsMap().at("latitude"s).AsDouble(), node.AsMap().at("longitude"s).AsDouble() };
        catalogue_.AddStop(node.AsMap().at("name"s).AsString(), coordinates);
    }
    // добавление дистанций
    for (json::Node& node : *stops) {
        std::string_view stop_from = node.AsMap().at("name"s).AsString();
        for (const std::pair<const std::string, json::Node>& stop_to : node.AsMap().at("road_distances"s).AsMap()) {
            catalogue_.AddDistances(stop_from, stop_to);
        }
    }
}

// добавление автобусов
void JsonReader::AddBuses(std::vector<json::Node>* buses) {
    using namespace std::literals;

    for (json::Node& node : *buses) {
        bool is_round = node.AsMap().at("is_roundtrip"s).AsBool();
        std::vector<std::string_view> stops;
        for (const json::Node& stop_node : node.AsMap().at("stops"s).AsArray()) {
            stops.push_back(stop_node.AsString());
        }
        if (!is_round) {
            std::vector<std::string_view> stops_copy(stops.begin(), stops.end());
            stops.insert(stops.end(), std::next(stops_copy.rbegin()), stops_copy.rend());
        }

        catalogue_.AddBus(node.AsMap().at("name"s).AsString(), move(stops), is_round);
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

    size_t requests_num = stat_requests->size();
    unsigned int request_counter = 0;

    json::Array root;
    root.reserve(requests_num);

    for (const json::Node& node : *stat_requests) {
        json::Dict one_responce;
        int id = node.AsMap().at("id"s).AsInt();

        if (node.AsMap().at("type"s).AsString() == "Bus"s) {
            if (std::optional<transport_catalogue::BusInfo> bus = catalogue->GetBusInfo(node.AsMap().at("name"s).AsString()); bus != std::nullopt) {

                one_responce["curvature"s] = json::Node(bus->curvature);
                one_responce["request_id"s] = json::Node(id);
                one_responce["route_length"s] = json::Node(bus->route_length);
                one_responce["stop_count"s] = json::Node(bus->stops_count);
                one_responce["unique_stop_count"s] = json::Node(bus->unique_stops_count);
            }
            else {
                one_responce["request_id"s] = json::Node(id);
                one_responce["error_message"s] = json::Node("not found"s);
            }
        }
        else {
            if (node.AsMap().at("type"s).AsString() == "Stop"s) {
                if (std::optional<std::set<std::string_view>> buses_of_stop = catalogue->GetStopInfo(node.AsMap().at("name"s).AsString()); buses_of_stop != std::nullopt) {
                    size_t buses_count = buses_of_stop->size();

                    json::Array buses;

                    for (const std::string_view& bus : *buses_of_stop) {
                        buses.emplace_back(json::Node(static_cast<std::string>(bus)));
                    }

                    one_responce["buses"s] = json::Node(buses);
                    one_responce["request_id"s] = json::Node(id);
                }
                else {
                    one_responce["request_id"s] = json::Node(id);
                    one_responce["error_message"s] = json::Node("not found"s);
                }
            }
            else {
                if (node.AsMap().at("type"s).AsString() == "Map"s) {

                    std::ostringstream outstream;

                    MapRenderer map_renderer(doc, catalogue);
                    map_renderer.CreateMap().RenderMap(outstream);

                    one_responce["map"s] = json::Node(outstream.str());
                    one_responce["request_id"s] = json::Node(id);
                }
                else {
                    throw std::invalid_argument("Invalid request type"s);
                }
            }
        }
        root.push_back(std::move(json::Node(std::move(one_responce))));
    }
    json::Document responce_doc(std::move(json::Node(std::move(root))));
    return responce_doc;
}

void JsonReader::PrintRequests(std::ostream& output) {
    using namespace std::literals;

    const json::Array* stat_requests = &doc_.GetRoot().AsMap().at("stat_requests"s).AsArray();
    json::Document responce = FormResponce(stat_requests, &catalogue_, &doc_);
    json::Print(responce, output);

}