#include <iostream>
#include <string>

//#include <fstream> // for tests

#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
    transport_catalogue::TransportCatalogue catalogue;

    //ifstream in_file("in.txt"s);
    //fstream out_file("out.txt"s, ios::in | ios::out | ios::trunc);
    int base_request_count;
    std::cin >> base_request_count >> ws;

    {
        transport_catalogue::input_processing::InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(std::cin, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    int stat_request_count;
    std::cin >> stat_request_count >> ws;
    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(std::cin, line);
        transport_catalogue::output_processing::ParseAndPrintStat(catalogue, line, std::cout);
    }
}