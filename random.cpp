#pragma once
#include <fstream>
#include <string>
using namespace std;

constexpr long MAXNUM = 10000000;

void create_random_input(string filename) {
    ofstream out;
    out.open(filename);

    for (long i = 0; i < MAXNUM; i++) {
        int buyOrSell = rand() % 2;
        int limitPrice = rand() % 1000;
        int shares = 1 + rand() % 100;
        out << (buyOrSell == 0 ? "B" : "S") << " " << limitPrice << " " << shares << "\n";
    }

    out.close();
}