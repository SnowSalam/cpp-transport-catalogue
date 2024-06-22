#pragma once

#include <algorithm>
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"
#include "json.h"
#include "domain.h"

namespace transport_catalogue {

	class TransportCatalogue {

	public:

		void AddStop(const std::string_view stop_name, geo::Coordinates coordinates);

		Stop* FindStop(std::string_view stop_name) const;

		std::optional<std::set<std::string_view>> GetStopInfo(std::string_view requested_stop) const;

		void AddBus(const std::string_view id, const std::vector<std::string_view>&& stops, bool is_roundtrip);

		const Bus* FindBus(std::string_view route_name) const;

		std::optional<BusInfo> GetBusInfo(std::string_view requested_bus) const;

		void AddDistances(const std::string_view stop_from, const std::pair<const std::string, json::Node> distances);

		int GetDistance(const std::string_view stop_from, const std::string_view stop_to) const;

		std::vector<const Bus*> GetBusCatalogue() const;

		std::vector<const Stop*> GetStopCatalogue() const;

		std::map<std::string_view, Stop*> GetSortedStops() const;

		std::map<std::string_view, Bus*> GetSortedBuses() const;

	private:
		class Hasher {
		public:
			size_t operator()(const std::pair<Stop*, Stop*>& pair) const;
		};

		std::deque<Stop> all_stops_;
		std::unordered_map<std::string_view, Stop*> stops_catalogue_;
		std::deque<Bus> all_buses_;
		std::unordered_map<std::string_view, Bus*> buses_catalogue_;
		std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;
		std::unordered_map<std::pair<Stop*, Stop*>, int, Hasher> distances_;


		int CountUniqueStops(std::string_view bus) const;
	};

}