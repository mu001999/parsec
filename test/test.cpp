#include <cctype>
#include <string>
#include <iostream>
#include "../src/parsec.hpp"

using namespace std;
using namespace parsec;

int main(int argc, char *argv[]) {

    Parsec<char> blank, blanks;
    Parsec<char> Decimal;
    Parsec<string> Number;
    Parsec<int> Primary, Additive, Additive_;

    blank = Token::by(::isblank);
    blanks = blank + blanks >> [](char, char) -> char { return 0; } | Token::epsilon<char>();

    // Decimal := '0' | ... | '9'
    Decimal = Token::by(::isdigit);

    // Number := Decimal Number | <epsilon>
    Number =
        Decimal + Number >>
            [](char decimal, string number) {
                return decimal + number;
            } |
        Token::epsilon<string>();

    // Primary := blanks Number
    Primary = blanks + Number >>
        [](char, string number) {
            return stoi(number);
        };

    // Additive := Primary Additive_
    // Additive_ := + Additive | - Additive | <epsilon>
    Additive = Primary + Additive_ >>
        [](int primary, int additive) {
            return primary + additive;
        };
    Additive_ =
        blanks + ('+'_T | '-'_T) + Additive >>
            [](char, char op, int additive) {
                return (op == '+' ? additive : -additive);
            } |
        Token::epsilon<int>();

    cout << Additive("1 + 2 + 3") << endl;

    return 0;
}
