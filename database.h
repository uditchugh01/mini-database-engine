#ifndef DATABASE_H
#define DATABASE_H

#include <fstream>
#include <string>
#include <vector>
#include "table.h"
#include "tokenizer.h"
using namespace std;

class Database {
public:
    Database();

    // Reads the database from disk (starts empty if there is no file yet)
    void loadFromFile();

    // Runs one command (already split into tokens) and auto-saves if it changed data
    void execute(vector<Token> toks);

private:
    vector<Table> tables;
    vector<Token> tokens;   // tokens of the command being executed
    int pos;                // index of the token we are looking at
    Token endToken;         // returned when we run past the last token
    string fileName;        // file where the database is stored
    bool changed;           // true if the last command modified the data

    // persistence
    bool saveToFile();
    void quarantineCorruptFile(ifstream& in);

    int findTable(string tName);

    // parsing helpers
    Token& cur();
    bool isKeyword(string kw);
    bool isSymbol(string s);
    bool expectKeyword(string kw);
    bool expectSymbol(string s);
    bool expectName(string& out, string what);
    bool expectEnd();
    bool parseWhere(Table& t, Condition& cond);

    // one function per command
    void doCreate();
    void doCreateIndex();
    void doInsert();
    void doSelect();
    void doDelete();
    void doUpdate();
};

#endif
