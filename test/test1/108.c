int valid_int = 42;          // Ordinary identifier that declares an object
int valid_function(void) {   // Ordinary identifier that declares a function
    return 0;
}

int valid_test(void) {
    int a = valid_int;        // 'valid_int' is a visible declaration
    int b = valid_function(); // 'valid_function' is a visible declaration
    return a + b;
}