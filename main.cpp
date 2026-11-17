#include <iostream>
#include <string>
#include <vector>
#include "database.h"
#include "tokenizer.h"
using namespace std;

int main() {
    Database db;
    string line;

    cout << "MiniDB - type SQL commands, or 'exit' to quit.\n";
    db.loadFromFile();

    while (true) {
        cout << "\nminidb> ";
        if (!getline(cin, line)) break;
        if (line == "exit") break;

        vector<Token> tokens;
        if (!tokenize(line, tokens)) continue;   // tokenizer already printed the error

        // Safety net: an unexpected exception should never crash the whole program
        try {
            db.execute(tokens);
        } catch (exception& e) {
            cout << "Internal error: " << e.what() << "\n";
        }
    }
    return 0;
}