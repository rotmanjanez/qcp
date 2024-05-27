// Invalid Example 1: Using block scope variable outside its scope
int foo(int val) {
    {
        int block_var = 10;
    }
    return block_var + val; // Error: block_var is not in scope here
}