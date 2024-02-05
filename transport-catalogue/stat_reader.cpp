#include "stat_reader.h"

using namespace std::string_literals;

std::string ParseRequest(std::string_view line) {
    auto second_word_end = line.find_last_not_of(' ');
    auto first_space = line.find(' ');
    auto second_word_begin = line.find_first_not_of(' ', first_space);
    return std::move(std::string(line.substr(second_word_begin, second_word_end - second_word_begin + 1)));
}

void PrintBusInfo(transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view requested_bus, std::ostream& output) {
    std::optional<transport_catalogue::BusInfo> bus_info = transport_catalogue.GetBusInfo(requested_bus);
    output << "Bus "s << requested_bus << ": "s;
    if (bus_info == std::nullopt) {
        output << "not found"s << std::endl;
    }
    else {
        output << bus_info->stops_count << " stops on route, "s
            << bus_info->unique_stops_count << " unique stops, "s << std::setprecision(6)
            << bus_info->route_length << " route length, "s
            << bus_info->curvature << " curvature"s << std::endl;
    }
}

void PrintStopInfo(transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view requested_stop, std::ostream& output) {
    std::optional<std::vector<std::string_view>> buses = transport_catalogue.GetStopInfo(requested_stop);
    output << "Stop "s << requested_stop << ": "s;
    if (buses != std::nullopt) {
        if (!buses->empty()) {
            output << "buses"s;
            for (std::string_view bus : *buses) {
                output << " " << bus;
            }
        }
        else {
            output << "no buses";
        }
    }
    else {
        output << "not found"s;
    }
    output << std::endl;
}

namespace transport_catalogue {
    namespace output_processing {

        void ParseAndPrintStat(transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view request,
            std::ostream& output) {
            std::string parsed_request = ParseRequest(request);

            if (request[0] == 'B') {
                PrintBusInfo(transport_catalogue, parsed_request, output);
            }
            if (request[0] == 'S') {
                PrintStopInfo(transport_catalogue, parsed_request, output);
            }
        }

    }

}