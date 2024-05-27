struct A {
    int x;
    void (*func)(struct A *self);
} globalA;