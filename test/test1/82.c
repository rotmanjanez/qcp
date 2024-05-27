int foo(void) {
    char *invalidChar = "\uDFFF"; // Code point DFFF is in the invalid range
    return 0;
}