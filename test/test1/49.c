typedef struct {
    int x;
} MyStruct;

MyStruct globalMyStruct;

struct MyStruct {
    int y;
} globalStruct; // Error: Conflicting declaration 'struct MyStruct'