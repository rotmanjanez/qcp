// Edge Case 1: Enumeration constant with same name in different enum
enum Color1 { RED, GREEN, BLUE };
enum Color2 { YELLOW, RED, ORANGE }; // 'RED' is redefined

int foo11() {
    return RED; // Ambiguous, depending on which 'RED' is in scope
}