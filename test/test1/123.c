int test3(void) {
    const int x = 10;
    x++; // Invalid: x is a const-qualified type and not a modifiable lvalue
    return 0;
}