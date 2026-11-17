#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include "btree.h"
using namespace std;

// A WHERE condition such as: id >= 5
struct Condition {
    bool active;       // false means there is no WHERE clause
    int colIndex;
    string op;         // =, !=, <, >, <=, >=
    string value;
};

class Table {
public:
    string name;
    vector<string> colNames;
    vector<string> colTypes;
    vector< vector<string> > rows;

    vector<int> indexedCols;     // which columns have an index
    vector<BTree> indexes;       // indexes[k] is the B-tree of column indexedCols[k]

    void addColumn(string cName, string cType);

    // Returns the index of the column, or -1 if it does not exist
    int findColumn(string cName);

    bool insertRow(vector<string> values);

    // ----- indexing -----
    bool createIndex(int colIndex);       // false if the column is already indexed
    int findIndex(int colIndex);          // position inside 'indexes', or -1
    void rebuildIndexes();                // needed after rows move or change

    // Checks whether a row satisfies the WHERE condition
    bool rowMatches(vector<string>& row, Condition& cond);

    // Returns the positions of all matching rows, in table order.
    // Uses an index when possible (usedIndex tells which way it was done).
    vector<int> getMatchingRows(Condition& cond, bool& usedIndex);

    void printRows(vector<int>& cols, Condition& cond);
    int deleteWhere(Condition& cond);
    int updateWhere(int colIndex, string newValue, Condition& cond);
};

#endif