int test4(void) {
    const int y = 20;
    const int *p = &y;
    (*p)++; // Invalid: *p is not a modifiable lvalue because y is const
    return 0;
}