// Invalid Example 2: Redefining a parameter in the function body
int foo(int val) {
    int val = 10; // Error: redeclaration of 'val'
    return val;
}