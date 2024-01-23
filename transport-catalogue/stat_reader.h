#pragma once

#include <iosfwd>
#include <string_view>
#include <iomanip>

#include "transport_catalogue.h"

namespace transport_catalogue {
    namespace output_processing {

        void ParseAndPrintStat(transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view request,
            std::ostream& output);

        namespace details {
            std::string ParseRequest(std::string_view line);
            void PrintBusInfo(transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view requested_bus, std::ostream& output);
            void PrintStopInfo(transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view requested_stop, std::ostream& output);
        }

    }
}