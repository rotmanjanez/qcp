// Example 3: File scope external variable
extern int external_var;

int foo(int val) {
    return external_var + val;
}

int external_var = 100;