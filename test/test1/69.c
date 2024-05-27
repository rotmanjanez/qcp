void foo(void) {
    int x = 10;
    void *vp = &x;  
    *vp = 20;  // Error: assigning to void expression
}