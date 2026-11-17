// Benchmark: full table scan vs B-tree index lookup.
// Build:  g++ -O2 -std=c++17 benchmark.cpp table.cpp btree.cpp utils.cpp -o benchmark

#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include "table.h"
using namespace std;

// Milliseconds elapsed since 'start'
double elapsedMs(chrono::steady_clock::time_point start) {
    chrono::duration<double, milli> d = chrono::steady_clock::now() - start;
    return d.count();
}

// Runs 'numQueries' lookups of the form  id = <some value>  and returns the total rows found
long long runQueries(Table& t, int numRows, int numQueries) {
    long long found = 0;
    for (int q = 0; q < numQueries; q++) {
        Condition cond;
        cond.active = true;
        cond.colIndex = 0;
        cond.op = "=";
        cond.value = to_string((long long)q * 7919 % numRows);   // spreads lookups over the whole table

        bool usedIndex;
        vector<int> result = t.getMatchingRows(cond, usedIndex);
        found += result.size();
    }
    return found;
}

int main() {
    int sizes[] = {10000, 100000, 500000};
    const int NUM_QUERIES = 100;

    cout << "Lookups of the form: SELECT * FROM t WHERE id = <value>\n";
    cout << NUM_QUERIES << " lookups per test\n\n";

    for (int s = 0; s < 3; s++) {
        int numRows = sizes[s];

        Table t;
        t.name = "bench";
        t.addColumn("id", "INT");
        t.addColumn("name", "TEXT");
        for (int i = 0; i < numRows; i++) {
            vector<string> row;
            row.push_back(to_string(i));
            row.push_back("user" + to_string(i));
            t.insertRow(row);
        }

        // 1) without an index (full scan)
        chrono::steady_clock::time_point start = chrono::steady_clock::now();
        long long found1 = runQueries(t, numRows, NUM_QUERIES);
        double scanMs = elapsedMs(start);

        // 2) build the index
        start = chrono::steady_clock::now();
        t.createIndex(0);
        double buildMs = elapsedMs(start);

        // 3) with the index
        start = chrono::steady_clock::now();
        long long found2 = runQueries(t, numRows, NUM_QUERIES);
        double indexMs = elapsedMs(start);

        cout << numRows << " rows\n";
        cout << "  full scan   : " << scanMs << " ms total, " << scanMs / NUM_QUERIES << " ms per lookup\n";
        cout << "  index build : " << buildMs << " ms (one time), tree height " << t.indexes[0].height() << "\n";
        cout << "  with index  : " << indexMs << " ms total, " << indexMs / NUM_QUERIES << " ms per lookup\n";
        if (indexMs > 0) cout << "  speed-up    : " << scanMs / indexMs << "x\n";
        if (found1 != found2) cout << "  ERROR: results differ!\n";
        cout << "\n";
    }
    return 0;
}