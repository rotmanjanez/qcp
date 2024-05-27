// Example 8: Parameter shadowing file scope variable
int file_var = 42;

int foo(int file_var) {
    return file_var * 2; // uses the parameter, not the file scope variable
}