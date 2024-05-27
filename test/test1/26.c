// File scope declaration
int x = 10;

int foo1(int val) {
    // Inner block scope declaration
    int x = 20;
    return x + val; // Uses the inner scope 'x', which is 20
}