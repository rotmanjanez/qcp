int test(void) {
    int x = 42;
    _Generic(x, 
             int: "int",
             nonexisting_type: "nonexisting" // Invalid: 'nonexisting_type' is not a type
    );
    return 0;
}