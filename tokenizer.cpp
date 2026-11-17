#include <iostream>
#include "tokenizer.h"

// These helpers are only used inside this file, so they are 'static' (private to the file)
static bool isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static void addToken(vector<Token>& tokens, string type, string text) {
    Token t;
    t.type = type;
    t.text = text;
    tokens.push_back(t);
}

bool tokenize(string input, vector<Token>& tokens) {
    tokens.clear();
    int i = 0;
    int n = input.size();

    while (i < n) {
        char c = input[i];

        // 1) Skip whitespace
        if (c == ' ' || c == '\t') {
            i++;
        }
        // 2) Word: keyword, table name or column name (INSERT, students, id ...)
        else if (isLetter(c)) {
            string word = "";
            while (i < n && (isLetter(input[i]) || isDigit(input[i]))) {
                word += input[i];
                i++;
            }
            addToken(tokens, "IDENT", word);
        }
        // 3) Number: digits with at most one decimal point (1, 85, 8.5)
        else if (isDigit(c)) {
            string num = "";
            bool seenDot = false;
            while (i < n && (isDigit(input[i]) || (input[i] == '.' && !seenDot))) {
                if (input[i] == '.') seenDot = true;
                num += input[i];
                i++;
            }
            addToken(tokens, "NUMBER", num);
        }
        // 4) String in double quotes: "Udit Kumar"
        else if (c == '"') {
            i++;   // skip the opening quote
            string s = "";
            while (i < n && input[i] != '"') {
                s += input[i];
                i++;
            }
            if (i >= n) {
                cout << "Error: string is missing a closing quote.\n";
                return false;
            }
            i++;   // skip the closing quote
            addToken(tokens, "STRING", s);
        }
        // 5) Two-character operators: !=  <=  >=
        else if ((c == '!' || c == '<' || c == '>') && i + 1 < n && input[i + 1] == '=') {
            string op = "";
            op += c;
            op += '=';
            addToken(tokens, "SYMBOL", op);
            i += 2;
        }
        // 6) Single-character symbols
        else if (c == '(' || c == ')' || c == ',' || c == '=' ||
                 c == '<' || c == '>' || c == '*' || c == ';') {
            string sym = "";
            sym += c;
            addToken(tokens, "SYMBOL", sym);
            i++;
        }
        // 7) Anything else is invalid
        else {
            cout << "Error: unexpected character '" << c << "'.\n";
            return false;
        }
    }
    return true;
}