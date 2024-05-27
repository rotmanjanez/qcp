int foo(void) {
    char *invalidChar = "\uD800"; // Code point D800 is in the invalid range
    return 0;
}