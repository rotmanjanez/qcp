struct A {
    int x;
    struct {
        int x; // Error: anonymous struct with member of same name
    }; 
} globalA;