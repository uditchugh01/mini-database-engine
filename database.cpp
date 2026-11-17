#include <iostream>
#include <cstdio>
#include "database.h"
#include "utils.h"

// ============================================================
// File-private helpers
// ============================================================

// Returns true if the token can be stored in a column of the given type
static bool valueFitsColumn(Token& tok, string colType) {
    if (colType == "INT") return tok.type == "NUMBER" && isValidInt(tok.text);
    if (colType == "TEXT") return tok.type == "STRING";
    return false;
}

// Prints a helpful message explaining what the column expects
static void printTypeError(string colName, string colType) {
    if (colType == "INT") {
        cout << "Error: column '" << colName << "' expects an INT (whole number, up to "
             << MAX_INT_DIGITS << " digits).\n";
    } else {
        cout << "Error: column '" << colName << "' expects TEXT in double quotes, e.g. \"abc\".\n";
    }
}

// Reads one line from the file and converts it to an int. Returns false on bad data.
static bool readInt(ifstream& in, int& value) {
    string line;
    if (!getline(in, line)) return false;
    if (!isNumber(line) || line.size() > 9) return false;
    value = stoi(line);
    return true;
}

// ============================================================
// Setup and persistence
// ============================================================

Database::Database() {
    pos = 0;
    fileName = "minidb.dat";
    changed = false;
    endToken.type = "END";
    endToken.text = "";
}

int Database::findTable(string tName) {
    for (int i = 0; i < tables.size(); i++) {
        if (tables[i].name == tName) return i;
    }
    return -1;
}

// Writes the whole database to disk. Returns true on success.
// It writes to a temporary file first, so a crash in the middle
// can never destroy the old data file.
bool Database::saveToFile() {
    string tempName = fileName + ".tmp";
    ofstream out(tempName.c_str());
    if (!out) {
        cout << "Error: could not write to '" << tempName << "'.\n";
        return false;
    }

    out << "MINIDB 2\n";
    out << tables.size() << "\n";
    for (int t = 0; t < tables.size(); t++) {
        Table& tbl = tables[t];
        out << tbl.name << "\n";
        out << tbl.colNames.size() << "\n";
        for (int c = 0; c < tbl.colNames.size(); c++) {
            out << tbl.colNames[c] << "\n";
            out << tbl.colTypes[c] << "\n";
        }
        out << tbl.rows.size() << "\n";
        for (int r = 0; r < tbl.rows.size(); r++) {
            for (int c = 0; c < tbl.rows[r].size(); c++) {
                out << tbl.rows[r][c] << "\n";
            }
        }
        // only the names of the indexed columns are stored; the trees are rebuilt on load
        out << tbl.indexedCols.size() << "\n";
        for (int k = 0; k < tbl.indexedCols.size(); k++) {
            out << tbl.colNames[tbl.indexedCols[k]] << "\n";
        }
    }
    out.close();
    if (out.fail()) {
        cout << "Error: failed while writing '" << tempName << "'.\n";
        return false;
    }

    remove(fileName.c_str());   // needed on Windows, where rename() will not overwrite
    if (rename(tempName.c_str(), fileName.c_str()) != 0) {
        cout << "Error: could not replace '" << fileName << "'.\n";
        return false;
    }
    return true;
}

// Moves a corrupted data file aside (so the next save cannot overwrite it)
void Database::quarantineCorruptFile(ifstream& in) {
    in.close();
    string backupName = fileName + ".corrupt";
    remove(backupName.c_str());
    if (rename(fileName.c_str(), backupName.c_str()) == 0) {
        cout << "Error: '" << fileName << "' is corrupted. It was moved to '" << backupName
             << "' and MiniDB started with an empty database.\n";
    } else {
        cout << "Error: '" << fileName << "' is corrupted and could not be moved aside.\n";
    }
}

void Database::loadFromFile() {
    ifstream in(fileName.c_str());
    if (!in) return;   // first run: no file yet

    vector<Table> loaded;   // fill a temporary list first, use it only if the whole file is valid
    string line;
    int numTables;

    if (!getline(in, line) || (line != "MINIDB 1" && line != "MINIDB 2")) {
        quarantineCorruptFile(in);
        return;
    }
    bool hasIndexSection = (line == "MINIDB 2");   // version 1 files have no index section

    if (!readInt(in, numTables)) {
        quarantineCorruptFile(in);
        return;
    }

    for (int t = 0; t < numTables; t++) {
        Table tbl;
        int numCols, numRows;

        if (!getline(in, tbl.name) || !readInt(in, numCols) || numCols == 0) {
            quarantineCorruptFile(in);
            return;
        }
        for (int c = 0; c < numCols; c++) {
            string colName, colType;
            if (!getline(in, colName) || !getline(in, colType) ||
                (colType != "INT" && colType != "TEXT")) {
                quarantineCorruptFile(in);
                return;
            }
            tbl.addColumn(colName, colType);
        }
        if (!readInt(in, numRows)) {
            quarantineCorruptFile(in);
            return;
        }
        for (int r = 0; r < numRows; r++) {
            vector<string> row;
            for (int c = 0; c < numCols; c++) {
                string value;
                // INT values must be valid numbers, otherwise comparisons would break later
                if (!getline(in, value) || (tbl.colTypes[c] == "INT" && !isValidInt(value))) {
                    quarantineCorruptFile(in);
                    return;
                }
                row.push_back(value);
            }
            tbl.rows.push_back(row);
        }

        if (hasIndexSection) {
            int numIndexes;
            if (!readInt(in, numIndexes)) {
                quarantineCorruptFile(in);
                return;
            }
            for (int k = 0; k < numIndexes; k++) {
                string colName;
                int col = -1;
                if (getline(in, colName)) col = tbl.findColumn(colName);
                if (col == -1 || tbl.colTypes[col] != "INT" || !tbl.createIndex(col)) {
                    quarantineCorruptFile(in);
                    return;
                }
            }
        }
        loaded.push_back(tbl);
    }

    tables = loaded;
    cout << "Loaded " << tables.size() << " table(s) from '" << fileName << "'.\n";
}

// ============================================================
// Parsing helpers
// ============================================================

Token& Database::cur() {
    if (pos < tokens.size()) return tokens[pos];
    return endToken;
}

bool Database::isKeyword(string kw) {
    return cur().type == "IDENT" && toUpper(cur().text) == kw;
}

bool Database::isSymbol(string s) {
    return cur().type == "SYMBOL" && cur().text == s;
}

bool Database::expectKeyword(string kw) {
    if (isKeyword(kw)) { pos++; return true; }
    cout << "Syntax error: expected " << kw << ".\n";
    return false;
}

bool Database::expectSymbol(string s) {
    if (isSymbol(s)) { pos++; return true; }
    cout << "Syntax error: expected '" << s << "'.\n";
    return false;
}

// Reads a table/column name into 'out'
bool Database::expectName(string& out, string what) {
    if (cur().type == "IDENT") {
        out = cur().text;
        pos++;
        return true;
    }
    cout << "Syntax error: expected " << what << ".\n";
    return false;
}

// The command must end here (an optional ';' is allowed)
bool Database::expectEnd() {
    if (isSymbol(";")) pos++;
    if (cur().type == "END") return true;
    cout << "Syntax error: unexpected '" << cur().text << "'.\n";
    return false;
}

// Parses an optional:  WHERE column operator value
bool Database::parseWhere(Table& t, Condition& cond) {
    cond.active = false;
    if (!isKeyword("WHERE")) return true;
    pos++;
    cond.active = true;

    string colName;
    if (!expectName(colName, "column name")) return false;
    cond.colIndex = t.findColumn(colName);
    if (cond.colIndex == -1) {
        cout << "Error: column '" << colName << "' not found.\n";
        return false;
    }

    string op = cur().text;
    bool validOp = (op == "=" || op == "!=" || op == "<" ||
                    op == ">" || op == "<=" || op == ">=");
    if (cur().type != "SYMBOL" || !validOp) {
        cout << "Syntax error: expected a comparison operator.\n";
        return false;
    }
    cond.op = op;
    pos++;

    if (!valueFitsColumn(cur(), t.colTypes[cond.colIndex])) {
        printTypeError(colName, t.colTypes[cond.colIndex]);
        return false;
    }
    cond.value = cur().text;
    pos++;
    return true;
}

// ============================================================
// Commands
// ============================================================

// CREATE TABLE name (col TYPE, col TYPE, ...)
void Database::doCreate() {
    if (isKeyword("INDEX")) {
        pos++;
        doCreateIndex();
        return;
    }
    if (!isKeyword("TABLE")) {
        cout << "Syntax error: expected TABLE or INDEX.\n";
        return;
    }
    pos++;
    string tableName;
    if (!expectName(tableName, "table name")) return;
    if (findTable(tableName) != -1) {
        cout << "Error: table '" << tableName << "' already exists.\n";
        return;
    }
    if (!expectSymbol("(")) return;

    Table t;
    t.name = tableName;
    while (true) {
        string colName, colType;
        if (!expectName(colName, "column name")) return;
        if (!expectName(colType, "column type")) return;
        colType = toUpper(colType);
        if (colType != "INT" && colType != "TEXT") {
            cout << "Error: unsupported type '" << colType << "'. Use INT or TEXT.\n";
            return;
        }
        if (t.findColumn(colName) != -1) {
            cout << "Error: duplicate column name '" << colName << "'.\n";
            return;
        }
        t.addColumn(colName, colType);
        if (isSymbol(",")) pos++;
        else break;
    }
    if (!expectSymbol(")")) return;
    if (!expectEnd()) return;

    tables.push_back(t);
    changed = true;
    cout << "Table '" << tableName << "' created.\n";
}

// INSERT INTO name VALUES (v1, v2, ...)
void Database::doInsert() {
    if (!expectKeyword("INTO")) return;
    string tableName;
    if (!expectName(tableName, "table name")) return;
    int ti = findTable(tableName);
    if (ti == -1) {
        cout << "Error: table '" << tableName << "' not found.\n";
        return;
    }
    if (!expectKeyword("VALUES")) return;
    if (!expectSymbol("(")) return;

    vector<Token> vals;
    while (true) {
        if (cur().type != "NUMBER" && cur().type != "STRING") {
            cout << "Syntax error: expected a value.\n";
            return;
        }
        vals.push_back(cur());
        pos++;
        if (isSymbol(",")) pos++;
        else break;
    }
    if (!expectSymbol(")")) return;
    if (!expectEnd()) return;

    Table& t = tables[ti];
    if (vals.size() != t.colNames.size()) {
        cout << "Error: expected " << t.colNames.size() << " values but got " << vals.size() << ".\n";
        return;
    }
    vector<string> row;
    for (int i = 0; i < vals.size(); i++) {
        if (!valueFitsColumn(vals[i], t.colTypes[i])) {
            printTypeError(t.colNames[i], t.colTypes[i]);
            return;
        }
        row.push_back(vals[i].text);
    }
    if (t.insertRow(row)) {
        changed = true;
        cout << "1 row inserted.\n";
    }
}

// SELECT * | col1, col2 FROM name [WHERE ...]
void Database::doSelect() {
    bool selectAll = false;
    vector<string> selectedNames;

    if (isSymbol("*")) {
        selectAll = true;
        pos++;
    } else {
        while (true) {
            string cn;
            if (!expectName(cn, "column name")) return;
            selectedNames.push_back(cn);
            if (isSymbol(",")) pos++;
            else break;
        }
    }

    if (!expectKeyword("FROM")) return;
    string tableName;
    if (!expectName(tableName, "table name")) return;
    int ti = findTable(tableName);
    if (ti == -1) {
        cout << "Error: table '" << tableName << "' not found.\n";
        return;
    }
    Table& t = tables[ti];

    vector<int> cols;   // which columns to print
    if (selectAll) {
        for (int i = 0; i < t.colNames.size(); i++) cols.push_back(i);
    } else {
        for (int i = 0; i < selectedNames.size(); i++) {
            int idx = t.findColumn(selectedNames[i]);
            if (idx == -1) {
                cout << "Error: column '" << selectedNames[i] << "' not found.\n";
                return;
            }
            cols.push_back(idx);
        }
    }

    Condition cond;
    if (!parseWhere(t, cond)) return;
    if (!expectEnd()) return;

    t.printRows(cols, cond);
}

// DELETE FROM name [WHERE ...]
void Database::doDelete() {
    if (!expectKeyword("FROM")) return;
    string tableName;
    if (!expectName(tableName, "table name")) return;
    int ti = findTable(tableName);
    if (ti == -1) {
        cout << "Error: table '" << tableName << "' not found.\n";
        return;
    }
    Table& t = tables[ti];

    Condition cond;
    if (!parseWhere(t, cond)) return;
    if (!expectEnd()) return;

    int deleted = t.deleteWhere(cond);
    changed = true;
    cout << deleted << " row(s) deleted.\n";
}

// UPDATE name SET column = value [WHERE ...]
void Database::doUpdate() {
    string tableName;
    if (!expectName(tableName, "table name")) return;
    int ti = findTable(tableName);
    if (ti == -1) {
        cout << "Error: table '" << tableName << "' not found.\n";
        return;
    }
    Table& t = tables[ti];

    if (!expectKeyword("SET")) return;
    string colName;
    if (!expectName(colName, "column name")) return;
    int idx = t.findColumn(colName);
    if (idx == -1) {
        cout << "Error: column '" << colName << "' not found.\n";
        return;
    }
    if (!expectSymbol("=")) return;
    if (!valueFitsColumn(cur(), t.colTypes[idx])) {
        printTypeError(colName, t.colTypes[idx]);
        return;
    }
    string newValue = cur().text;
    pos++;

    Condition cond;
    if (!parseWhere(t, cond)) return;
    if (!expectEnd()) return;

    int updated = t.updateWhere(idx, newValue, cond);
    changed = true;
    cout << updated << " row(s) updated.\n";
}

// CREATE INDEX ON name (column)
void Database::doCreateIndex() {
    if (!expectKeyword("ON")) return;
    string tableName;
    if (!expectName(tableName, "table name")) return;
    int ti = findTable(tableName);
    if (ti == -1) {
        cout << "Error: table '" << tableName << "' not found.\n";
        return;
    }
    if (!expectSymbol("(")) return;
    string colName;
    if (!expectName(colName, "column name")) return;
    if (!expectSymbol(")")) return;
    if (!expectEnd()) return;

    Table& t = tables[ti];
    int idx = t.findColumn(colName);
    if (idx == -1) {
        cout << "Error: column '" << colName << "' not found.\n";
        return;
    }
    if (t.colTypes[idx] != "INT") {
        cout << "Error: only INT columns can be indexed.\n";
        return;
    }
    if (!t.createIndex(idx)) {
        cout << "Error: column '" << colName << "' is already indexed.\n";
        return;
    }

    changed = true;
    int height = t.indexes[t.findIndex(idx)].height();
    cout << "Index created on " << tableName << "(" << colName << "): "
         << t.rows.size() << " row(s), tree height " << height << ".\n";
}

// Entry point: looks at the first word and calls the matching function
void Database::execute(vector<Token> toks) {
    tokens = toks;
    pos = 0;
    if (tokens.empty()) return;

    if (isKeyword("CREATE")) { pos++; doCreate(); }
    else if (isKeyword("INSERT")) { pos++; doInsert(); }
    else if (isKeyword("SELECT")) { pos++; doSelect(); }
    else if (isKeyword("DELETE")) { pos++; doDelete(); }
    else if (isKeyword("UPDATE")) { pos++; doUpdate(); }
    else cout << "Error: unknown command '" << tokens[0].text << "'.\n";

    // Save to disk after every command that modified the data
    if (changed) {
        saveToFile();
        changed = false;
    }
}