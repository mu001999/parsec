# Parsec

A parser combinator library for C++, you can copy the header file to `/usr/include` to include it easily

## Example

```cpp
Parsec<char> Blank, Blanks;
Parsec<char> Decimal;
Parsec<string> Number;
Parsec<int> Primary, Additive, Additive_;

Blank = Token::by(::isspace);
Blanks = Blank + Blanks >> [](char, char) -> char { return 0; } | Token::epsilon<char>();

// Decimal := '0' | ... | '9'
Decimal = Token::by(::isdigit);

// Number := Decimal Number | <epsilon>
Number =
    Decimal + Number >>
        [](char decimal, string number) {
            return decimal + number;
        } |
    Token::epsilon<string>();

// Primary := Blanks Number
Primary = Blanks + Number >>
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
    Blanks + ('+'_T | '-'_T) + Additive >>
        [](char, char op, int additive) {
            return (op == '+' ? additive : -additive);
        } |
    Token::epsilon<int>();

cout << Additive("1 + 2 + 3") << endl;
```

## ChangeLog

+ [X] Fix bug if the return type is copy unassignable
+ [X] Support \<epsilon\>
