// Invalid Example 1: Using prototype scope variable outside its scope
int foo6(int x);

int foo6(int x) {
    int y = x + z; // Error: 'z' is not declared
    return y;
}