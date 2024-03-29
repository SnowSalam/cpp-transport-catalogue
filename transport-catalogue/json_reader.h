#pragma once

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

class JsonReader {
public:
    JsonReader(std::istream& input)
        : doc_(json::Load(input)) {
        FillCatalogue();
        SetRouterSettings();
    }

    void PrintToStream(std::ostream& output = std::cout) {
        PrintRequests(output);
    }

private:
    json::Document doc_;
    transport_catalogue::TransportCatalogue catalogue_;
    TransportRouter router_;

    // сортировка запросов на добавление в каталог
    void SortInputRequests(std::vector<json::Node>& buses, std::vector<json::Node>& stops);

    // добавление остановок и дистанций
    void AddStops(std::vector<json::Node>* stops);

    // добавление автобусов
    void AddBuses(std::vector<json::Node>* buses);

    void FillCatalogue();
    void SetRouterSettings();

    void PrintRequests(std::ostream& output);
    json::Document FormResponce(const json::Array* stat_requests);
};