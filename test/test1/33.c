// Edge Case 2: Shadowing with loop variable
int foo10(int val) {
    int e = 180;
    for (int e = 0; e < 5; e++) { // Loop scope variable shadows outer 'e'
        val += e; // Uses the loop scope 'e'
    }
    return val + e; // Uses the outer scope 'e', which is 180
}