int foo(void) {
    char *invalidChar = "\U00110000"; // Code point 110000 is greater than 10FFFF
    return 0;
}