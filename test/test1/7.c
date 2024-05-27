// Example 5: Block scope shadowing file scope variable
int file_var = 42;

int foo(int val) {
    int file_var = 10; // shadows the file scope file_var
    return file_var + val;
}