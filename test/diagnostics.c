int foo() {
   return;
}
/* ------- SEPERATOR ------- */
void foo() {
   return 10;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10;
}
/* ------- SEPERATOR ------- */
int foo() int x = 10;
return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   x = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
foo() {
   int x = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo( {
   int x = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int arr[];
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int arr[-5];
   return 0;
}
/* ------- SEPERATOR ------- */
int foo();
int foo(int x) {
   return x;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10;
   return 0;
}
}
/* ------- SEPERATOR ------- */
int foo() {
    int arr[10;
    return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   char c = "A";
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int arr[10.5];
   return 0;
}
/* ------- SEPERATOR ------- */
int foo();
int foo(x, y) {
   return x + y;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x;
   x.y = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
struct A {
   int x;
} globalA;

int x = globalA.x;
/* ------- SEPERATOR ------- */
int foo(int val) {
   int val = 10;
   return val;
}
/* ------- SEPERATOR ------- */
int[] foo() {
   static int arr[10];
   return arr;
}
/* ------- SEPERATOR ------- */
int foo(int arg) {} // Error: end of non void function
/* ------- SEPERATOR ------- */
int foo(int x);
int foo(float x) {
   return (int) x;
}
/* ------- SEPERATOR ------- */
int foo() {
   return 10;
}
int foo() {
   return 20;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10;
   int x = 20;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10;
   int y = *x;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   goto label;
   int x = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   arr[0] = 1;
   int arr[10];
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   void* ptr;
   int x = *ptr;
   return 0;
}
/* ------- SEPERATOR ------- */
void foo() {
   void x; // Error: declaring void object
}
/* ------- SEPERATOR ------- */
int foo() {
   const int x = 10;
   x = 20;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int arr[3];
   int arr = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
int test5(void) {
   int arr[10];
   arr++;
   return 0;
}
/* ------- SEPERATOR ------- */
struct Point;
int foo() {
   struct Point p;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int arr[10];
   arr[2.5] = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
struct Point {
   x;
   int y;
};
int foo() {
   return 0;
}
/* ------- SEPERATOR ------- */
struct Point {
   int x;
   y;
};
int foo() {
   return 0;
}
/* ------- SEPERATOR ------- */
int test3(void) {
   const int x = 10;
   x++;
   return 0;
}
/* ------- SEPERATOR ------- */
struct Point {
   int x;
   int y;
} int foo() {
   return 0;
}
/* ------- SEPERATOR ------- */
struct Point {
   int x;
   int x;
};
int foo() {
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   float x = 10.5;
   float y = x & 2;
   return 0;
}
/* ------- SEPERATOR ------- */
typedef int myint;
int foo() {
   myfloat x = 10.0;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int bar() {
      return 10;
   }
   return bar();
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 1;
   case 1:
      x += 1;
      return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 1;
   default:
      x += 1;
      return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   break;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   void x;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   continue;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo();
char foo() {
   return 'a';
}
/* ------- SEPERATOR ------- */
int foo() {
   goto label;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = bar();
   return 0;
}
int bar() {
   return 10;
}
/* ------- SEPERATOR ------- */
int foo(int x) {
   int x = 1; // Error, x already defined
   return x;
}
/* ------- SEPERATOR ------- */
int foo() {
   return 10;
}
int bar() {
   char (*ptr)() = foo;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   return 10;
}
int bar() {
   int (*ptr)() = (char (*)()) foo;
   return 0;
}
/* ------- SEPERATOR ------- */
int test(void) {
   int arr[10];
   int* ptr = arr;
   int value = ptr[ptr];
   return 0;
}
/* ------- SEPERATOR ------- */
struct Point {
   int x;
};
int foo() {
   struct Point p = {.x = 10, .y = 20};
   return 0;
}
/* ------- SEPERATOR ------- */
struct Point {
   int x;
   int y;
};
int foo() {
   struct Point p;
   p.z = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10;
   switch (x) {
        case 1
            break;
    }
    return 0;
}
/* ------- SEPERATOR ------- */
struct Node {
   struct Node next;
   int data;
} globalNode; // Error: Incomplete type 'struct Node'
/* ------- SEPERATOR ------- */
void foo(void) {
   int x = 10;
   void* vp = &x;
   *vp = 20; // Error: assigning to void expression
}
/* ------- SEPERATOR ------- */
// Edge Case 4: Invalid Function prototype
int foo(void, int);

int foo(void, int i) {
   return i;
}
/* ------- SEPERATOR ------- */
struct Incomplete;

struct Incomplete test(struct Incomplete* arr) {
   int index = 5;
   return arr[index];
}
/* ------- SEPERATOR ------- */
enum Colors { RED,
              GREEN,
              BLUE };
int foo() {
   Colors color = RED;
   return 0;
}
/* ------- SEPERATOR ------- */
struct A {
   struct {
      int x;
      struct {
         int y;
      };
   };
} globalA;

int y = globalA.y;
/* ------- SEPERATOR ------- */
struct A {
   int x;
   struct {
      int x; // Error: anonymous struct with member of same name
   };
} globalA;
/* ------- SEPERATOR ------- */
enum Colors { RED,
              GREEN,
              BLUE };
int foo() {
   enum Colors color = YELLOW;
   return 0;
}
/* ------- SEPERATOR ------- */
union Data {
   int x;
   char x;
};
/* ------- SEPERATOR ------- */
int foo(int val) {
   return undeclared_var + val;
}

int undeclared_var = 50;
/* ------- SEPERATOR ------- */
int foo(int x) {
   return x;
}
int bar() {
   int y = foo(1, 2);
   return 0;
}
/* ------- SEPERATOR ------- */
int test(void) {
   int a = 5;
   int b = 10;
   int value = a[b];
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 1;
   int y = 2;
   switch (x) {
      case y:
           x = 3;
           break;
   }
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10;
   switch (x) {
      case 1:
         break;
      case 1:
         break;
   }
   return 0;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   int arr[3] = {1, 2, 3};
   arr = &val; // Error: assignment to expression with array type
   return arr[0];
}
/* ------- SEPERATOR ------- */
struct A {
   int x;
   struct B {
      int y;
   } b;
} globalA1;

struct A {
   int z;
} globalA2; // Error: Redefinition of 'struct A'
/* ------- SEPERATOR ------- */
int foo() {
   int x = 1;
   switch (x) {
      case 1:
         break;
      default:
         break;
      default:
         break;
   }
   return 0;
}
/* ------- SEPERATOR ------- */
typedef struct {
   int x;
} MyStruct;

MyStruct globalMyStruct;

struct MyStruct {
   int y;
} globalStruct; // Error: Conflicting declaration 'struct MyStruct'
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10 return 0;
}
/* ------- SEPERATOR ------- */
enum Colors { RED,
              GREEN,
              RED };
int foo() {
   return 0;
}
/* ------- SEPERATOR ------- */
int foo(int x, int y) {
   return x + y;
}
int bar() {
   int z = foo(1);
   return 0;
}
/* ------- SEPERATOR ------- */
int test(void) {
   int x = 42;
   _Generic(x,
       int: "int",
       nonexisting_type: "nonexisting");
   return 0;
}
/* ------- SEPERATOR ------- */
// Invalid Example 2: Conflicting declarations in the same scope
int foo(int val) {
   int d = 150;
   int d = 160; // Error: redeclaration of 'd'
   return d;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   {
      int block_var = 10;
   }
   return block_var + val;
}
/* ------- SEPERATOR ------- */
struct Point {
   int x;
   int y;
};
union Data {
   struct Point point;
   int z;
};
int foo() {
   struct Point p;
   union Data data;
   p = data;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo(int x);

int foo(int x) {
   int y = x + z; // Error: 'z' is not declared
   return y;
}
/* ------- SEPERATOR ------- */
int foo(int x);

int foo(int y) { // Error: 'x' is not declared in this scope
   return x * y;
}
/* ------- SEPERATOR ------- */
int foo(int x, int x); // Error: redeclaration of parameter 'x'

int foo(int x, int y) {
   return x + y;
}
