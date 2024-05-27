// Invalid Example 3: Using undeclared variable in function definition
int foo8(int x);

int foo8(int y) { // Error: 'x' is not declared in this scope
    return x * y;
}