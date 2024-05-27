int foo(void) {
    char *validChar = "\U0001F600"; // Code point 1F600 is valid (represents 😀)
    return 0;
}