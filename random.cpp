#pragma once
#include <fstream>
#include <string>
using namespace std;

constexpr long MAXNUM = 1000000;


void create_random_input(string filename) {
    ofstream out;
    out.open(filename + ".txt");

    for (int i = 0; i < MAXNUM; i++) {
        int buyOrSell = rand() % 2;
        int limitPrice = rand() % 1000000;
        int shares = rand() % 1000000;
        out << (buyOrSell == 0 ? "B" : "S") << " " << limitPrice << " " << shares << "\n";
    }

    out.close();
}