// Example 1: Simple enumeration
enum Color { RED, GREEN, BLUE };

int foo1() {
    enum Color color = GREEN; // 'GREEN' is in scope here
    return color;
}