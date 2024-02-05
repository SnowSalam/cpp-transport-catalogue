#pragma once

#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string route_name;
		std::vector<Stop*> route;
	};

	struct BusInfo {
		int stops_count;
		int unique_stops_count;
		int route_length = 0;
		double curvature = 0.0;
	};

	class TransportCatalogue {

	public:

		void AddStop(const std::string_view stop_name, geo::Coordinates coordinates);

		const Stop* FindStop(std::string_view stop_name) const;

		std::optional<std::vector<std::string_view>> GetStopInfo(std::string_view requested_stop) const;

		void AddBus(const std::string_view id, const std::vector<std::string_view>& stops);

		const Bus* FindBus(std::string_view route_name) const;

		std::optional<BusInfo> GetBusInfo(std::string_view requested_bus) const;

		void AddDistances(const std::string_view stop, std::vector<std::pair<std::string, int>> distances);

	private:
		std::deque<Stop> all_stops_;
		std::unordered_map<std::string_view, Stop*> stops_catalogue_;
		std::deque<Bus> all_buses_;
		std::unordered_map<std::string_view, Bus*> buses_catalogue_;
		std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;

		class Hasher {
		public:
			size_t operator()(const std::pair<Stop*, Stop*>& pair) const;
		};

		std::unordered_map<std::pair<Stop*, Stop*>, int, Hasher> distances_;

		int CountUniqueStops(std::string_view bus) const;
	};

}