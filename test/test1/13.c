// Invalid Example 3: Using a file scope variable before declaration
int foo(int val) {
    return undeclared_var + val; // Error: undeclared_var is not declared yet
}

int undeclared_var = 50;