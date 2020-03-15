#include <string>
#include <iostream>
#include "../src/parsec.hpp"

using namespace std;
using namespace parsec;

int main(int argc, char *argv[])
{
    Parsec<char> Decimal;
    Parsec<string> Number;
    Parsec<int> Primary, Additive, Additive_;

    // Decimal := '0' | ... | '9'
    Decimal = ('0'_T | '1'_T | '2'_T | '3'_T | '4'_T | '5'_T | '6'_T | '7'_T | '8'_T | '9'_T );

    // Number := Decimal Number | Decimal
    Number =
        (Decimal + Number >>
            [](char decimal, string number) {
                return decimal + number;
            }) |
        (Decimal >>
            [](char decimal) {
                return string() + decimal;
            });

    // Primary := Number
    Primary = Number >> [](string number) {
        return stoi(number);
    };

    // Additive := Primary Additive_ | Primary
    // Additive_ := + Additive | - Additive
    Additive =
        (Primary + Additive_ >>
            [](int primary, int additive) {
                return primary + additive;
            }) |
        (Primary >> [](int primary) {
            return primary;
        });
    Additive_ = (('+'_T | '-'_T) + Additive >> [](char op, int additive) {
        return (op == '+' ? additive : -additive);
    });

    cout << Additive("1+2+3") << endl;

    return 0;
}
