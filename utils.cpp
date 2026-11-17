#include "utils.h"

bool isNumber(string s) {
    if (s.empty()) return false;
    for (int i = 0; i < s.size(); i++) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

bool isValidInt(string s) {
    return isNumber(s) && s.size() <= MAX_INT_DIGITS;
}

string toUpper(string s) {
    for (int i = 0; i < s.size(); i++) {
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = s[i] - 'a' + 'A';
    }
    return s;
}