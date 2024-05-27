// Example 5: Prototype scope shadowing file scope variable
int global_var = 10;

int foo5(int global_var); // Shadows the file scope global_var

int foo5(int global_var) {
    return global_var * 2;
}