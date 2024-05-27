struct A {
    int x;
    struct B {
        int y;
    } b;
} globalA1;

struct A {
    int z;
} globalA2; // Error: Redefinition of 'struct A'