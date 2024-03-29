#include "transport_catalogue.h"

namespace transport_catalogue {

	void TransportCatalogue::AddStop(const std::string_view stop_name, geo::Coordinates coordinates) {
		all_stops_.emplace_back(std::string(stop_name), coordinates);
		Stop* current_stop = &all_stops_.back();
		stops_catalogue_[current_stop->name] = current_stop;
		stops_to_buses_[current_stop->name];
	}

	Stop* TransportCatalogue::FindStop(std::string_view stop_name) const {
		if (stops_catalogue_.find(stop_name) != stops_catalogue_.end()) {
			return stops_catalogue_.at(stop_name);
		}
		return nullptr;
	}

	int TransportCatalogue::GetDistance(const std::string_view stop_from, const std::string_view stop_to) const {
		Stop* from = FindStop(stop_from);
		Stop* to = FindStop(stop_to);
		if (auto it = distances_.find({ from, to }); it != distances_.end()) {
			return it->second;
		}
		else {
			if (stop_from == stop_to) return 0;
			return distances_.at({ to, from });
		}
	}

	std::optional<std::set<std::string_view>> TransportCatalogue::GetStopInfo(std::string_view requested_stop) const {
		if (FindStop(requested_stop) != nullptr) {
			std::set<std::string_view> buses;
			if (stops_to_buses_.find(requested_stop) != stops_to_buses_.end()) {
				for (std::string_view bus : stops_to_buses_.at(requested_stop)) {
					buses.insert(bus);
				}
			}
			return buses;
		}
		else {
			return std::nullopt;
		}
	}

	void TransportCatalogue::AddBus(const std::string_view id, const std::vector<std::string_view>&& stops, bool is_roundtrip) {
		std::vector<Stop*> route_stops;
		for (const std::string_view& stop : stops) {
			if (stops_catalogue_.find(stop) != stops_catalogue_.end()) {
				route_stops.push_back(stops_catalogue_[stop]);
			}
		}
		all_buses_.emplace_back(std::string(id), std::move(route_stops), is_roundtrip);
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

			BusInfo bus_info(bus->route.size(), CountUniqueStops(requested_bus), road_route_distance, curvature, bus->is_roundtrip);
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

	void TransportCatalogue::AddDistances(const std::string_view stop_from, const std::pair<const std::string, json::Node> distances) {
			distances_[std::pair{ stops_catalogue_.at(stop_from), stops_catalogue_.at(distances.first) }] = distances.second.AsInt();
	}

	std::vector<const Bus*> TransportCatalogue::GetBusCatalogue() const {
		std::vector<const Bus*> buses;
		for (const auto& pair : buses_catalogue_) {
			buses.push_back(pair.second);
		}
		std::sort(buses.begin(), buses.end(),
			[](const Bus* lhs, const Bus* rhs) {
				return lhs->route_name < rhs->route_name;
			});
		return buses;
	}

	bool CompareStops(const Stop* s1, const Stop* s2) {
		return s1->name == s2->name;
	}

	std::vector<const Stop*> TransportCatalogue::GetStopCatalogue() const {
		std::vector<const Stop*> stops;
		for (const Bus& bus : all_buses_) {
			if (!bus.route.empty()) {
				for (const Stop* stop : bus.route) {
					stops.push_back(stop);
				}
			}
		}
		std::sort(stops.begin(), stops.end(),
			[](const Stop* lhs, const Stop* rhs) {
				return lhs->name < rhs->name;
			});
		stops.erase(std::unique(stops.begin(), stops.end(), CompareStops), stops.end());
		return stops;
	}

	size_t TransportCatalogue::Hasher::operator()(const std::pair<Stop*, Stop*>& pair) const {
		return (size_t)std::hash<const void*>{}(pair.first) * 17 + (size_t)std::hash<const void*>{}(pair.second) * 23;
	}

	std::map<std::string_view, Stop*> TransportCatalogue::GetSortedStops() const {
		return { stops_catalogue_.begin(), stops_catalogue_.end() };
	}

	std::map<std::string_view, Bus*> TransportCatalogue::GetSortedBuses() const {
		return { buses_catalogue_.begin(), buses_catalogue_.end() };
	}
}