int test(void) {
    int x = 42;
    _Generic(x, 
             int: "int",
             signed int: "signed int" // Invalid: 'int' and 'signed int' are compatible types
    );
    return 0;
}