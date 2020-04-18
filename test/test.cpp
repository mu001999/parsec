#include <cctype>
#include <string>
#include <iostream>
#include "../src/parsec.hpp"

using namespace std;
using namespace parsec;

int main(int argc, char *argv[])
{
    Parsec<char> Blank, Blanks;
    Parsec<char> Decimal;
    Parsec<string> Number;
    Parsec<int> Primary, Product, Additive;

    Blank = Token::by(::isspace);
    Blanks =
      Blank + Blanks >>
        [](char, char) -> char
        {
            return 0;
        } |
      Token::epsilon<char>();

    // Decimal
    // : '0' | ... | '9'
    Decimal = Token::by(::isdigit);

    // Number
    // : Decimal Number | <epsilon>
    Number =
      Decimal + Number >>
        [](char decimal, string number)
        {
            return decimal + number;
        } |
      Token::epsilon<string>();

    // Primary
    // : Blanks Number
    Primary =
      Blanks + Number >>
        [](char, string number)
        {
            return stoi(number);
        };

    // Product
    // : Primary Blanks * Product
    // | Primary Blanks / Product
    // | Primary
    Product =
      Primary + Blanks + '*'_T + Product >>
        [](int lhs, char, char, int rhs)
        {
            return lhs * rhs;
        } |
      Primary + Blanks + '/'_T + Product >>
        [](int lhs, char, char, int rhs)
        {
            return lhs / rhs;
        } |
      Primary;

    // Additive
    // : Product + Additive
    // | Product - Additive
    // | Product
    Additive =
      Product + Blanks + '+'_T + Additive >>
        [](int lhs, char, char, int rhs)
        {
            return lhs + rhs;
        } |
      Product + Blanks + '-'_T + Additive >>
        [](int lhs, char, char, int rhs)
        {
            return lhs - rhs;
        } |
      Product;

    cout << Additive("2 * 3 + 4 * 5") << endl;

    return 0;
}
