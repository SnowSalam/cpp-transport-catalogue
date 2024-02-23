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

void JsonReader::PrintRequests(std::ostream& output) {
    using namespace std::literals;

    const json::Array* stat_requests = &doc_.GetRoot().AsMap().at("stat_requests"s).AsArray();
    size_t requests_num = stat_requests->size();
    unsigned int request_counter = 0;

    output << "[\n"s;
    for (const json::Node& node : *stat_requests) {
        int id = node.AsMap().at("id"s).AsInt();

        if (node.AsMap().at("type"s).AsString() == "Bus"s) {
            if (std::optional<transport_catalogue::BusInfo> bus = catalogue_.GetBusInfo(node.AsMap().at("name"s).AsString()); bus != std::nullopt) {

                output << "    {\n\t\"curvature\": " << bus->curvature << ",\n";
                output << "\t\"request_id\": " << id << ",\n";
                output << "\t\"route_length\": " << bus->route_length << ",\n";
                output << "\t\"stop_count\": " << bus->stops_count << ",\n";
                output << "\t\"unique_stop_count\": " << bus->unique_stops_count << "\n    }";
            }
            else {
                output << "    {\n\t\"request_id\": " << id << ",\n\t\"error_message\": \"not found\"\n    }";
            }
        }
        else {
            if (node.AsMap().at("type"s).AsString() == "Stop"s) {
                if (std::optional<std::set<std::string_view>> buses_of_stop = catalogue_.GetStopInfo(node.AsMap().at("name"s).AsString()); buses_of_stop != std::nullopt) {
                    size_t buses_count = buses_of_stop->size();
                    unsigned int counter = 0;

                    output << "    {\n\t\"buses\": [\n\t";
                    for (const std::string_view& bus : *buses_of_stop) {
                        output << "\"" << bus << "\"";
                        ++counter;
                        if (counter < buses_count) output << ",";
                        output << "\n\t";
                    }
                    output << "],\n\t\"request_id\": " << id << "\n    }";


                }
                else {
                    output << "    {\n\t\"request_id\": " << id << ",\n\t\"error_message\": \"not found\"\n    }";

                }
            }
            else {
                if (node.AsMap().at("type"s).AsString() == "Map"s) {
                    output << "    {\n\t\"map\": \"";
                    CreateMap(&doc_, &catalogue_, output);
                    output << "\",\n\t\"request_id\": " << id << "\n    }";

                }
                else {
                    throw std::invalid_argument("Invalid request type"s);
                }
            }
        }
        ++request_counter;
        if (request_counter < requests_num) output << ",\n";
    }
    output << "\n]" << std::endl;
}