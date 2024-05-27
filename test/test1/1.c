// Example 1: File scope variable
int file_var = 42;

int foo(int val) {
    return file_var + val;
}