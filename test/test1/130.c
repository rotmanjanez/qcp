int foo() {
    int i = 0;
    {
        static int i = 0;
        ++i;
    }
    return i;
}