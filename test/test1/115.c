int test(void) {
    double x = 42.0;
    _Generic(x, 
             int: "int",
             float: "float" // Invalid: 'double' is not compatible with any named type and there is no default
    );
    return 0;
}