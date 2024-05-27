struct A {
    int x;
    struct B {
        int y;
    } b;
} globalA;

struct C {
    struct A a;
    struct B {
        int z;
    } b;
} globalC;