struct incomplete_type;

int test(void) {
    int x = 42;
    _Generic(x, 
             int: "int",
             incomplete_type: "incomplete" // Invalid: 'incomplete_type' is not a complete type
    );
    return 0;
}