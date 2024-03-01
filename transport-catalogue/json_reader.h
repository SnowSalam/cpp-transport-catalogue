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

class JsonReader {
public:
    JsonReader(std::istream& input)
        : doc_(json::Load(input)) {
        FillCatalogue();
    }

    void PrintToStream(std::ostream& output = std::cout) {
        PrintRequests(output);
    }

private:
    json::Document doc_;
    transport_catalogue::TransportCatalogue catalogue_;

    // сортировка запросов на добавление в каталог
    void SortInputRequests(std::vector<json::Node>& buses, std::vector<json::Node>& stops);

        // добавление остановок и дистанций
    void AddStops(std::vector<json::Node>* stops);

        // добавление автобусов
    void AddBuses(std::vector<json::Node>* buses);

    void FillCatalogue();

    void PrintRequests(std::ostream& output);
};