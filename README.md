# Parsec

A parser combinator library for C++, you can copy the header file to `/usr/include` to include it easily

## Example

```cpp
Parsec<char> Decimal;
Parsec<string> Number;
Parsec<int> Primary, Additive, Additive_;

// Decimal := '0' | ... | '9'
Decimal = '0'_T | '1'_T | '2'_T | '3'_T | '4'_T | '5'_T | '6'_T | '7'_T | '8'_T | '9'_T;

// Number := Decimal Number | <epsilon>
Number =
    Decimal + Number >>
        [](char decimal, string number) {
            return decimal + number;
        } |
    Token::epsilon<string>();

// Primary := Number
Primary = Number >>
    [](string number) {
        return stoi(number);
    };

// Additive := Primary Additive_
// Additive_ := + Additive | - Additive | <epsilon>
Additive = Primary + Additive_ >>
    [](int primary, int additive) {
        return primary + additive; };
Additive_ =
    ('+'_T | '-'_T) + Additive >>
        [](char op, int additive) {
            return (op == '+' ? additive : -additive); } |
    Token::epsilon<int>();

cout << Additive("1+2+3") << endl;
```

## ChangeLog

+ [X] Support \<epsilon\>
