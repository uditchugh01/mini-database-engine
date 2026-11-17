#ifndef UTILS_H
#define UTILS_H

#include <string>
using namespace std;

const int MAX_INT_DIGITS = 18;   // keeps INT values safely inside the range of long long

// Returns true if the string is non-empty and contains only digits
bool isNumber(string s);

// Returns true if the string is a digits-only number that fits in an INT column
bool isValidInt(string s);

// Converts a-z to A-Z
string toUpper(string s);

#endif