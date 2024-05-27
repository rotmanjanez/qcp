struct A; // Forward declaration

struct B {
    struct A *a;
} globalB;

struct A {
    struct B *b;
} globalA;