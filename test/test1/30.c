// Invalid Example 2: Conflicting declarations in the same scope
int foo8(int val) {
    int d = 150;
    int d = 160; // Error: redeclaration of 'd'
    return d;
}