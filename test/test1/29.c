int foo4(int val) {
    int z = 70;
    {
        int z = 80; // Inner block scope
        {
            int z = 90; // Innermost block scope
            val += z; // Uses the innermost scope 'z', which is 90
        }
        val += z; // Uses the inner scope 'z', which is 80
    }
    val += z; // Uses the outer scope 'z', which is 70
    return val;
}