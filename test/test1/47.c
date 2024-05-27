struct A {
    int x;
    struct B {
        int y;
    } b;
} globalA;

void func() {
    struct B {
        float z;
    } b;
}