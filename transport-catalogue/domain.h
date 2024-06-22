#pragma once

#include <string>
#include <vector>

namespace geo {
    struct Coordinates {

        double lat; // Latitude
        double lng; // Longitude
    };
}

namespace transport_catalogue {
	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string route_name;
		std::vector<Stop*> route;
        bool is_roundtrip;
	};

	struct BusInfo {
		int stops_count;
		int unique_stops_count;
		int route_length = 0;
		double curvature = 0.0;
        bool is_roundtrip;
	};
}

namespace svg {
    struct Rgb {
        Rgb() = default;
        Rgb(uint8_t red, uint8_t green, uint8_t blue) : red(red), green(green), blue(blue) {}

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba {
        Rgba() = default;
        Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity)
            : red(red), green(green), blue(blue), opacity(opacity) {}

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };
}