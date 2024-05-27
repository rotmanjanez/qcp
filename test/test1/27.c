int foo2(int x) {
    {
        // Inner block scope declaration
        int x = 30;
        return x; // Uses the inner scope 'x', which is 30
    }
    // Returns the outer scope 'x' (function parameter) if the inner block is not executed
    return x; // This would use the parameter 'x'
}