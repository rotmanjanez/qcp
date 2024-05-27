struct A {
    struct {
        int x;
        struct {
            int y;
        };
    };
} globalA;


int x = globalA.x;
int y = globalA.y;