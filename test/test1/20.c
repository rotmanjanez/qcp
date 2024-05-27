// Invalid Example 2: Function prototype scope variable conflict
int foo7(int x, int x); // Error: redeclaration of parameter 'x'

int foo7(int x, int y) {
    return x + y;
}