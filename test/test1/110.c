int test(void) {
    int x = 42;
    _Generic(x, 
             default: "default1",
             default: "default2" // Invalid: more than one default generic association
    );
    return 0;
}