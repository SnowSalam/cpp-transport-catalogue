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
		double route_length = 0.0;
		for (int i = 1, size = route_stops.size(); i < size; ++i) {
			route_length += geo::ComputeDistance(route_stops[i - 1]->coordinates, route_stops[i]->coordinates);
		}
		all_buses_.push_back({ std::string(id), route_stops, route_length });
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
			BusInfo bus_info(bus->route.size(), CountUniqueStops(requested_bus), bus->route_length);
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

}