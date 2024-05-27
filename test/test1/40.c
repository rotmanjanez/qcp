// Example 3: Nested enumeration scope
enum Outer { A, B, C };

int foo3() {
    enum Inner { A = 10, B = 20 }; // Shadows 'A' and 'B' from enum Outer
    return A + B + C; // Uses 'A' and 'B' from Inner, 'C' from Outer
}