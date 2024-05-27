// imao this should not be possible, but it is :)
int foo(int x), bar(int x, int y);

int foo(int x) {
    return x;
}

int bar(int x, int y) {
    return x + y;
}