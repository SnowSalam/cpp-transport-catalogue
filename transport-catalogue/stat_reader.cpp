#include "stat_reader.h"

using namespace std::string_literals;

namespace transport_catalogue {
    namespace output_processing {

        void ParseAndPrintStat(transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view request,
            std::ostream& output) {
            std::string parsed_request = details::ParseRequest(request);

            if (request[0] == 'B') {
                details::PrintBusInfo(transport_catalogue, parsed_request, output);
            }
            if (request[0] == 'S') {
                details::PrintStopInfo(transport_catalogue, parsed_request, output);
            }
        }

        namespace details {

            std::string ParseRequest(std::string_view line) {
                auto second_word_end = line.find_last_not_of(' ');
                auto first_space = line.find(' ');
                auto second_word_begin = line.find_first_not_of(' ', first_space);
                return std::move(std::string(line.substr(second_word_begin, second_word_end - second_word_begin + 1)));
            }

            void PrintBusInfo(transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view requested_bus, std::ostream& output) {
                std::tuple<int, int, double> bus_info = transport_catalogue.GetBusInfo(requested_bus); // { количество остановок, уникальные остановки, длина маршрута }
                output << "Bus "s << requested_bus << ": "s;
                if (std::get<0>(bus_info) == -1) {
                    output << "not found"s << std::endl;
                }
                else {
                    output << std::get<0>(bus_info) << " stops on route, "s
                        << std::get<1>(bus_info) << " unique stops, "s << std::setprecision(6)
                        << std::get<2>(bus_info) << " route length"s << std::endl;
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

        }
    }
}