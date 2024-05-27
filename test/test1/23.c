// Edge Case 2: Function prototype with complex types
int foo10(int a, int (*func)(int, float));

int foo10(int a, int (*func)(int, float)) {
    return func(a, 3.14);
}