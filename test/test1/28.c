int foo3(int val) {
    int y = 40;
    {
        int y = 50; // Inner block scope
        {
            int y = 60; // Innermost block scope
            return y; // Uses the innermost scope 'y', which is 60
        }
        return y; // Uses the inner scope 'y', which is 50
    }
    return y; // Uses the outer scope 'y', which is 40
}