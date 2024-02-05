#include "transport_catalogue.h"

namespace transport_catalogue {

	void TransportCatalogue::AddStop(const std::string_view stop_name, geo::Coordinates coordinates) {
		Stop stop{ std::string(stop_name), coordinates };
		all_stops_.push_back(std::move(stop));
		Stop* current_stop = &all_stops_.back();
		stops_catalogue_[current_stop->name] = current_stop;
		stops_to_buses_[current_stop->name];
	}

	const Stop* TransportCatalogue::FindStop(std::string_view stop_name) const {
		if (stops_catalogue_.find(stop_name) != stops_catalogue_.end()) {
			return stops_catalogue_.at(stop_name);
		}
		return nullptr;
	}

	std::optional<std::vector<std::string_view>> TransportCatalogue::GetStopInfo(std::string_view requested_stop) const {
		if (FindStop(requested_stop) != nullptr) {
			std::vector<std::string_view> buses;
			if (stops_to_buses_.find(requested_stop) != stops_to_buses_.end()) {
				for (std::string_view bus : stops_to_buses_.at(requested_stop)) {
					buses.push_back(bus);
				}
			}
			return buses;
		}
		else {
			return std::nullopt;
		}
	}

	void TransportCatalogue::AddBus(const std::string_view id, const std::vector<std::string_view>& stops) {
		std::vector<Stop*> route_stops;
		for (const std::string_view& stop : stops) {
			if (stops_catalogue_.find(stop) != stops_catalogue_.end()) {
				route_stops.push_back(stops_catalogue_[stop]);
			}
		}
		all_buses_.push_back({ std::string(id), route_stops });
		buses_catalogue_[all_buses_.back().route_name] = &all_buses_.back();
		for (const std::string_view& stop : stops) {
			stops_to_buses_[stop].insert(all_buses_.back().route_name);
		}
	}

	const Bus* TransportCatalogue::FindBus(std::string_view route_name) const {
		if (buses_catalogue_.find(route_name) == buses_catalogue_.end()) {
			return nullptr;
		}
		return buses_catalogue_.at(route_name);
	}

	std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view requested_bus) const {
		if (const Bus* bus = FindBus(requested_bus); bus != nullptr) {

			double geo_route_length = 0.0;
			int road_route_distance = 0;
			for (int i = 1, size = bus->route.size(); i < size; ++i) {
				geo_route_length += geo::ComputeDistance(bus->route[i - 1]->coordinates, bus->route[i]->coordinates);

				if (distances_.find({ bus->route[i - 1], bus->route[i] }) != distances_.end()) {
					road_route_distance += distances_.at({ bus->route[i - 1], bus->route[i] });
				}
				else {
					road_route_distance += distances_.at({ bus->route[i], bus->route[i - 1] });
				}
			}

			double curvature = static_cast<double>(road_route_distance) / geo_route_length;

			BusInfo bus_info(bus->route.size(), CountUniqueStops(requested_bus), road_route_distance, curvature);
			return bus_info;
		}
		else {
			return std::nullopt;
		}
	}

	int TransportCatalogue::CountUniqueStops(std::string_view bus) const {
		std::unordered_set<Stop*> unique_stops;
		for (Stop* stop : buses_catalogue_.at(bus)->route) {
			unique_stops.insert(stop);
		}
		return unique_stops.size();
	}

	void TransportCatalogue::AddDistances(const std::string_view stop_from, std::vector<std::pair<std::string, int>> distances) {
		for (auto [stop_to, distance] : distances) {
			distances_[std::pair{ stops_catalogue_.at(stop_from), stops_catalogue_.at(stop_to) }] = distance;
		}
	}

	size_t TransportCatalogue::Hasher::operator()(const std::pair<Stop*, Stop*>& pair) const {
		return (size_t)std::hash<const void*>{}(pair.first) * 17 + (size_t)std::hash<const void*>{}(pair.second) * 23;
	}
}