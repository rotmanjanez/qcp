// Example 6: Block scope variable in nested blocks
int foo(int val) {
    int outer_var = 5;
    {
        int inner_var = 10;
        outer_var += inner_var;
    }
    return outer_var + val;
}