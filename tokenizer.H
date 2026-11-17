#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>
using namespace std;

struct Token {
    string type;   // IDENT, NUMBER, STRING or SYMBOL
    string text;
};

// Splits a command into tokens.
// Returns false (and prints an error) if the input contains invalid characters.
bool tokenize(string input, vector<Token>& tokens);

#endif