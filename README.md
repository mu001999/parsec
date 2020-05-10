# Parsec

A parser combinator library for C++, you can copy the header file to `/usr/include` to include it easily

## Example

See [test/test.cpp](test/test.cpp)

## To Do

- [ ] Update to be used with not only string but also other types which could be traversaled

## ChangeLog

- [X] Support memorization to avoid repeated analysis, making grammar definition clearer
- [X] Fix bug if the return type is copy unassignable
- [X] Support \<epsilon\>
