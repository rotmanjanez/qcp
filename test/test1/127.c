int test6(void) {
    volatile int x = 10;
    x++; // Valid: x is a qualified real type and a modifiable lvalue
    return x;
}