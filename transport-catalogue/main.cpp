#include <iostream>
#include <string>
#include <sstream>
#include <set>

//#include <fstream> // for tests

#include "json_reader.h"

using namespace std;

int main() {
    //ifstream input("in.txt"s);

    JsonReader json_doc(std::cin);

    json_doc.PrintToStream(std::cout);
}