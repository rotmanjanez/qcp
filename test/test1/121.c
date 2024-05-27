int test1(void) {
    int x = 10;
    x++; // Valid: x is an unqualified real type and a modifiable lvalue
    return x;
}