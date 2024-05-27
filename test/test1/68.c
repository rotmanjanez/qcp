struct MyStruct {
    int member;
};

int foo(int val) {
    struct MyStruct s;  // s.member is an lvalue and designates an object of type int
    s.member = val;     // using s.member as an lvalue
    return s.member;    // using s.member as an lvalue
}