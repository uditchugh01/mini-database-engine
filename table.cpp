#include <iostream>
#include <algorithm>
#include <climits>
#include "table.h"
#include "utils.h"

void Table::addColumn(string cName, string cType) {
    colNames.push_back(cName);
    colTypes.push_back(cType);
}

int Table::findColumn(string cName) {
    for (int i = 0; i < colNames.size(); i++) {
        if (colNames[i] == cName) return i;
    }
    return -1;
}

bool Table::insertRow(vector<string> values) {
    if (values.size() != colNames.size()) {
        cout << "Error: expected " << colNames.size() << " values.\n";
        return false;
    }
    for (int i = 0; i < values.size(); i++) {
        if (colTypes[i] == "INT" && !isValidInt(values[i])) {
            cout << "Error: column '" << colNames[i] << "' expects an INT value.\n";
            return false;
        }
    }
    rows.push_back(values);

    // keep every index up to date: the new row sits at position rows.size() - 1
    for (int k = 0; k < indexes.size(); k++) {
        indexes[k].insert(stoll(values[indexedCols[k]]), rows.size() - 1);
    }
    return true;
}

// ---------- indexing ----------

int Table::findIndex(int colIndex) {
    for (int k = 0; k < indexedCols.size(); k++) {
        if (indexedCols[k] == colIndex) return k;
    }
    return -1;
}

bool Table::createIndex(int colIndex) {
    if (findIndex(colIndex) != -1) return false;

    indexedCols.push_back(colIndex);
    indexes.push_back(BTree());
    BTree& tree = indexes.back();
    for (int r = 0; r < rows.size(); r++) {
        tree.insert(stoll(rows[r][colIndex]), r);
    }
    return true;
}

// Row positions change after DELETE, so the simplest safe fix is to rebuild
void Table::rebuildIndexes() {
    for (int k = 0; k < indexes.size(); k++) {
        indexes[k].clear();
        for (int r = 0; r < rows.size(); r++) {
            indexes[k].insert(stoll(rows[r][indexedCols[k]]), r);
        }
    }
}

// ---------- searching ----------

bool Table::rowMatches(vector<string>& row, Condition& cond) {
    if (!cond.active) return true;   // no WHERE means every row matches

    string cell = row[cond.colIndex];
    int cmp;   // -1 if cell < value, 0 if equal, 1 if cell > value

    if (colTypes[cond.colIndex] == "INT") {
        long long a = stoll(cell);
        long long b = stoll(cond.value);
        if (a < b) cmp = -1;
        else if (a > b) cmp = 1;
        else cmp = 0;
    } else {
        if (cell < cond.value) cmp = -1;
        else if (cell > cond.value) cmp = 1;
        else cmp = 0;
    }

    if (cond.op == "=") return cmp == 0;
    if (cond.op == "!=") return cmp != 0;
    if (cond.op == "<") return cmp < 0;
    if (cond.op == ">") return cmp > 0;
    if (cond.op == "<=") return cmp <= 0;
    if (cond.op == ">=") return cmp >= 0;
    return false;
}

vector<int> Table::getMatchingRows(Condition& cond, bool& usedIndex) {
    vector<int> result;
    usedIndex = false;

    // Fast path: the condition is on an indexed column (and is not '!=')
    if (cond.active && cond.op != "!=") {
        int k = findIndex(cond.colIndex);
        if (k != -1) {
            long long v = stoll(cond.value);
            long long lo = LLONG_MIN;
            long long hi = LLONG_MAX;

            if (cond.op == "=")  { lo = v;     hi = v; }
            if (cond.op == "<")  { hi = v - 1; }
            if (cond.op == "<=") { hi = v; }
            if (cond.op == ">")  { lo = v + 1; }
            if (cond.op == ">=") { lo = v; }

            indexes[k].rangeSearch(lo, hi, result);
            sort(result.begin(), result.end());   // the tree gives key order; we want table order
            usedIndex = true;
            return result;
        }
    }

    // Slow path: look at every row
    for (int r = 0; r < rows.size(); r++) {
        if (rowMatches(rows[r], cond)) result.push_back(r);
    }
    return result;
}

void Table::printRows(vector<int>& cols, Condition& cond) {
    for (int i = 0; i < cols.size(); i++) cout << colNames[cols[i]] << "\t";
    cout << "\n-------------------------\n";

    bool usedIndex;
    vector<int> matches = getMatchingRows(cond, usedIndex);

    for (int m = 0; m < matches.size(); m++) {
        int r = matches[m];
        for (int c = 0; c < cols.size(); c++) cout << rows[r][cols[c]] << "\t";
        cout << "\n";
    }
    cout << "(" << matches.size() << " row(s))";
    if (usedIndex) cout << " [used index]";
    cout << "\n";
}

// ---------- modification ----------

int Table::deleteWhere(Condition& cond) {
    int count = 0;
    for (int r = 0; r < rows.size(); ) {
        if (rowMatches(rows[r], cond)) {
            rows.erase(rows.begin() + r);   // do not advance r: the next row shifts into this position
            count++;
        } else {
            r++;
        }
    }
    if (count > 0) rebuildIndexes();   // row positions changed
    return count;
}

int Table::updateWhere(int colIndex, string newValue, Condition& cond) {
    int count = 0;
    for (int r = 0; r < rows.size(); r++) {
        if (rowMatches(rows[r], cond)) {
            rows[r][colIndex] = newValue;
            count++;
        }
    }
    // only an update of an indexed column can make an index outdated
    if (count > 0 && findIndex(colIndex) != -1) rebuildIndexes();
    return count;
}