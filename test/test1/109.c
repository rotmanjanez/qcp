int invalid_test(void) {
    int a = invalid_int;      // 'invalid_int' has no visible declaration (should cause a compilation error)
    int b = invalid_function(); // 'invalid_function' has no visible declaration (should cause a compilation error)
    return a + b;
}