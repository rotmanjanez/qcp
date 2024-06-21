int i = 42;
/* ------- SEPERATOR ------- */
int i[];

int i[3];
/* ------- SEPERATOR ------- */
int* p = (int[]){1, 2, 3};
/* ------- SEPERATOR ------- */
int foo(int a) {
   return -a;
}
/* ------- SEPERATOR ------- */
int foo_plus(int a) {
   return +a;
}
/* ------- SEPERATOR ------- */
bool foo_not(int a) {
   return !a;
}
/* ------- SEPERATOR ------- */
int foo(int bar) {
   return +bar--;
}
/* ------- SEPERATOR ------- */
long foo_long(long a) {
   return -a;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   return val * 2;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10;
   return x;
}
/* ------- SEPERATOR ------- */
float foo_plus(float a) {
   return +a;
}
/* ------- SEPERATOR ------- */
double invalid_hex_double_df = 0x1.8p3df;
/* ------- SEPERATOR ------- */
double invalid_hex_double_dd = 0x1.8p3dd;
/* ------- SEPERATOR ------- */
double invalid_hex_double_dl = 0x1.8p3dl;
/* ------- SEPERATOR ------- */
double invalid_hex_double_DF = 0x1.8p3DF;
/* ------- SEPERATOR ------- */
double invalid_hex_double_DD = 0x1.8p3DD;
/* ------- SEPERATOR ------- */
double invalid_hex_double_DL = 0x1.8p3DL;
/* ------- SEPERATOR ------- */
int foo(void);

int foo() {
   return 1;
}
/* ------- SEPERATOR ------- */
int foo();

int foo(void) {
   return 1;
}
/* ------- SEPERATOR ------- */
bool foo_not_bool(bool a) {
   return !a;
}
/* ------- SEPERATOR ------- */
int foo() {
   int* ptr = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = foo();
   return 0;
}
/* ------- SEPERATOR ------- */
int foo_bitwise_not(int a) {
   return ~a;
}
/* ------- SEPERATOR ------- */
int foo() {
   int arr[10];
   return arr;
}
/* ------- SEPERATOR ------- */
void func() {
   int* p = (int[]){4, 5, 6};
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10 / 0;
   return 0;
}
/* ------- SEPERATOR ------- */
bool foo_not_float(float a) {
   return !a;
}
/* ------- SEPERATOR ------- */
double foo_double(double a) {
   return -a;
}
/* ------- SEPERATOR ------- */
int foo_dereference(int* a) {
   return *a;
}
/* ------- SEPERATOR ------- */
int foo() {
   void* ptr = 10;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = "Hello";
   return 0;
}
/* ------- SEPERATOR ------- */
int foo(void);

int foo(void) {
   return 42;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   int x = val;
   return x;
}
/* ------- SEPERATOR ------- */
double foo_plus_double(double a) {
   return +a;
}
/* ------- SEPERATOR ------- */
int foo_prefix_increment(int a) {
   return ++a;
}
/* ------- SEPERATOR ------- */
int foo_prefix_decrement(int a) {
   return --a;
}
/* ------- SEPERATOR ------- */
int foo(int x);

int foo(int x) {
   return x * 2;
}
/* ------- SEPERATOR ------- */
unsigned long ul = 1UL;
int s = 2;
typeof(ul * s) uls;
/* ------- SEPERATOR ------- */
unsigned int ua = 1;
int sa = 2;
typeof(ua * sa) uasa;
/* ------- SEPERATOR ------- */
int test1(void) {
   int x = 10;
   x++;
   return x;
}
/* ------- SEPERATOR ------- */
unsigned foo_sizeof_int(int a) {
   return sizeof(a);
}
/* ------- SEPERATOR ------- */
unsigned short us = 1;
int si = 2;
typeof(us * si) ussi;
/* ------- SEPERATOR ------- */
unsigned int ui = 1;
short ss = 2;
typeof(ui * ss) uiss;
/* ------- SEPERATOR ------- */
int foo(int a, ...);

int foo(int a, ...) {
   return a;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
label1:
   return val;
   goto label1;
}
/* ------- SEPERATOR ------- */
unsigned foo_sizeof_float(float a) {
   return sizeof(a);
}
/* ------- SEPERATOR ------- */
int foo() {
   char str[10];
   int x = str;
   return 0;
}
/* ------- SEPERATOR ------- */
int* foo_address(int a) {
   int* ptr = &a;
   return ptr;
}
/* ------- SEPERATOR ------- */
int foo(int x) {
   {
      int x = 1;
      return x;
   }
}
/* ------- SEPERATOR ------- */
int foo(int bar) {
   if (bar)
      return 1;
   return 0;
}
/* ------- SEPERATOR ------- */
int foo(void) {
   char* validChar = "\u0041";
   return 0;
}
/* ------- SEPERATOR ------- */
struct Node {
   int data;
   struct Node* next;
} globalNode;
/* ------- SEPERATOR ------- */
int foo(void) {
   char* invalidChar = "\uD800";
   return 0;
}
/* ------- SEPERATOR ------- */
int foo(void) {
   char* invalidChar = "\uDFFF";
   return 0;
}
/* ------- SEPERATOR ------- */
typeof('\??"\"') c = '\??"\"';
typeof("\??'\'") str = "\??'\'";
/* ------- SEPERATOR ------- */
struct A {
   int x;
   struct {
      int x;
   } b;
} globalA;
/* ------- SEPERATOR ------- */
struct A {
   int x;
   void (*func)(struct A* self);
} globalA;
/* ------- SEPERATOR ------- */
typedef int myint;
typedef int myint;
int foo() {
   return 0;
}
/* ------- SEPERATOR ------- */
enum Number {
   ONE = 1,
   TWO,
   THREE
};

int number = TWO;
/* ------- SEPERATOR ------- */
int foo_postfix_decrement(int a) {
   int b = a--;
   return b;
}
/* ------- SEPERATOR ------- */
int foo_postfix_increment(int a) {
   int b = a++;
   return b;
}
/* ------- SEPERATOR ------- */
int foo(void) {
   char* validChar = "\U0001F600";
   return 0;
}
/* ------- SEPERATOR ------- */
int file_var = 42;

int foo(int val) {
   return file_var + val;
}
/* ------- SEPERATOR ------- */
int foo(int bar) {
   if (bar != 0)
      return 1;
   return 0;
}
/* ------- SEPERATOR ------- */
int x = 10;

int foo(int val) {
   int x = 20;
   return x + val;
}
/* ------- SEPERATOR ------- */
int foo(void) {
   char* invalidChar = "\U00110000";
   return 0;
}
/* ------- SEPERATOR ------- */
int file_var = 42;

int foo(int file_var) {
   return file_var * 2;
}
/* ------- SEPERATOR ------- */
int external_var;

int foo(int val) {
   return external_var + val;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   int block_var = 10;
   return block_var + val;
}
/* ------- SEPERATOR ------- */
unsigned int foo_bitwise_not_unsigned(unsigned int a) {
   return ~a;
}
/* ------- SEPERATOR ------- */
int test2(void) {
   int arr[10];
   int* p = arr;
   p++;
   return *p;
}
/* ------- SEPERATOR ------- */
int foo(int x) {
   {
      int x = 30;
      return x;
   }
   return x;
}
/* ------- SEPERATOR ------- */
int foo(int arg) {
   const int value = 0;
   value = arg;
   return arg;
}
/* ------- SEPERATOR ------- */
int foo(int arg) {
   const int value = 0;
   value = arg;
   return arg;
}
/* ------- SEPERATOR ------- */
int single_i = ’\x123’;
int double_i = \0223’;

wchar_t double_w = '\0223’;
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

unsigned char invalid_oct = '\400';
/* ------- SEPERATOR ------- */
extern int external_var;

int foo(int val) {
   return external_var + val;
}
/* ------- SEPERATOR ------- */
int a = 1, b = 2;
typeof(a * b) ab;
/* ------- SEPERATOR ------- */
float foo(float a) {
   return -a;
}
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

char8_t invalid_utf8 = u8'\x80\x80';
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

unsigned char invalid_hex = '\x100';
/* ------- SEPERATOR ------- */
int foo(int val) {
   goto forward_label;
forward_label:
   return val * 2;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   goto skip_init;
   int x = val;
skip_init:
   return x;
}
/* ------- SEPERATOR ------- */
int test4(void) {
   const int y = 20;
   int* p = &y;
   (*p)++;
   return 0;
}
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

char16_t invalid_utf16 = u'\x8000\x8000';
/* ------- SEPERATOR ------- */
int foo(int val) {
   int arr[3] = {1, 2, 3};
   arr[1] = val;
   return arr[1];
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   {
      goto out_of_block;
   }
out_of_block:
   return val;
}
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

    wchar_t invalid_wchar = L'\x8000\x8000';
/* ------- SEPERATOR ------- */
int foo() {
   int i = 0;
   {
      static int i = 0;
      ++i;
   }
   return i;
}
/* ------- SEPERATOR ------- */
struct S {
   int n;
   int arr[];
};

struct S s = {3, {1, 2, 3}};
struct S globalS;
/* ------- SEPERATOR ------- */
int test4(void) {
   const int y = 20;
   const int* p = &y;
   (*p)++;
   return 0;
}
/* ------- SEPERATOR ------- */
struct A;
struct B {
   struct A* a;
} globalB;

struct A {
   struct B* b;
} globalA;
/* ------- SEPERATOR ------- */
int file_var = 42;

int foo(int val) {
   int file_var = 10;
   return file_var + val;
}
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

char32_t invalid_utf32 = U'\x80000000\x80000000';
/* ------- SEPERATOR ------- */
struct foo {
   int a;
};
void test() {
   struct foo fooInstance;
   fooInstance.a = 10;
}
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

char* invalid5 = "Hello, "
                 L"World!";
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

char16_t* invalid3 = u"Hello, "
                     U"World!";
/* ------- SEPERATOR ------- */
int foo() {
   return __func__[0] == 'f' && __func__[1] == 'f' && __func__[2] == 'f' && __func__[3] == 0;
}
/* ------- SEPERATOR ------- */
enum Colors { RED,
              GREEN,
              BLUE };
int foo() {
   int color = RED;
   return 0;
}
/* ------- SEPERATOR ------- */
int test(void) {
   double x = 42.0;
   _Generic(x,
       int: "int",
       float: "float");
   return 0;
}
/* ------- SEPERATOR ------- */
int foo(int a, int (*func)(int, float));

int foo(int a, int (*func)(int, float)) {
   return func(a, 3.14);
}
/* ------- SEPERATOR ------- */
void voidFunction() {}

int intFunction() {}

void foo(void) {
   (void) voidFunction()(void) intFunction();
}
/* ------- SEPERATOR ------- */
int foo(int x), bar(int x, int y);

int foo(int x) {
   return x;
}

int bar(int x, int y) {
   return x + y;
}
/* ------- SEPERATOR ------- */
int foo() {
   int 变量 = 10;
   int _anotherVar = 20;
   int $money = 30;

   return 变量 + _anotherVar + $money;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   if (val == 1) goto label;
   if (val == 2) goto label;
   return -1;
label:
   return val;
}
/* ------- SEPERATOR ------- */
int test(void) {
   int x = 42;
   _Generic(x,
       int: "int",
       signed int: "signed int");
   return 0;
}
/* ------- SEPERATOR ------- */
int foo() {
   char x = 'a';
   switch (x) {
      case 'a':
         x = 'b';
         break;
   }
   return 0;
}
/* ------- SEPERATOR ------- */
long double ld = 1.0L;
double d = 2.0;
float f = 3.0F;

typeof(ld * d) ldd;
typeof(ld * f) ldf;

typeof(d * f) df;
/* ------- SEPERATOR ------- */
int test(void) {
   int arr[10];
   _Generic(arr,
       int*: "int pointer",
       float: "float");
   return 0;
}
/* ------- SEPERATOR ------- */
int test(void) {
   int x = 42;
   _Generic(x,
       default: "default1",
       default: "default2");
   return 0;
}
/* ------- SEPERATOR ------- */
struct MyStruct {
   int member;
};

int foo(int val) {
   struct MyStruct s;
   s.member = val;
   return s.member;
}
/* ------- SEPERATOR ------- */
enum Color { RED,
             GREEN,
             BLUE };

int foo() {
   enum Color color = GREEN;
   return color;
}
/* ------- SEPERATOR ------- */
struct A {
   int x;
   struct B {
      int y;
   } b;
} globalA;

void func() {
   struct B {
      float z;
   } b;
}
/* ------- SEPERATOR ------- */
int file_function(int x);

int foo(int val) {
   return file_function(val);
}

int file_function(int x) {
   return x * 2;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   int i = 0;
start_loop:
   if (i >= val) goto end_loop;
   i++;
   goto start_loop;
end_loop:
   return i;
}
/* ------- SEPERATOR ------- */
int foo(int bar) {
   return -bar;
}
/* ------- SEPERATOR ------- */
double foo(double a, int b);

double foo(double a, int b) {
   return a * b;
}
/* ------- SEPERATOR ------- */
int foo(int arr[10]);

int foo(int arr[10]) {
   return arr[0] + sizeof(arr);
}
/* ------- SEPERATOR ------- */
int test(void) {
   int arr[10];
   int index = 5;
   int value = arr[index];
   return value;
}
/* ------- SEPERATOR ------- */
int foo(int x, float y, char z);

int foo(int x, float y, char z) {
   return x + (int) y + z;
}
/* ------- SEPERATOR ------- */
struct Point {
   int x, y;
};
int foo() {
   struct Point p = {.x = 10, .y = 20};
   return 0;
}
/* ------- SEPERATOR ------- */
int invalid_test(void) {
   int a = invalid_int;
   int b = invalid_function();
   return a + b;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   int e = 180;
   for (int e = 0; e < 5; e++) { val += e; }
   return val + e;
}
/* ------- SEPERATOR ------- */
int global_var = 10;

int foo(int global_var);
int foo(int global_var) {
   return global_var * 2;
}
/* ------- SEPERATOR ------- */
extern int external_var;

int foo(int val) {
   return external_var + val;
}

int external_var = 100;
/* ------- SEPERATOR ------- */
int foo(int val) {
   if (val < 0) goto end_function;
   return val * 2;
end_function:
   return 0;
}
/* ------- SEPERATOR ------- */
union Bar {
   int a;
   float b;
};

void test() {
   union Bar barInstance;
   barInstance.a = 5;
}
/* ------- SEPERATOR ------- */
enum Color {
   RED,
   GREEN,
   BLUE
};

void test() {
   enum Color myColor;
   myColor = GREEN;
}
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

wchar_t* invalid2 = L"Hello, "
                    u"World!";
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

char32_t* invalid4 = U"Hello, "
                     "World!";
/* ------- SEPERATOR ------- */
enum MyEnum { ONE = 1,
              TWO = 2 } e = ONE;
int i = 3;

typeof(e * i) ei;
typeof(i * e) ie;
/* ------- SEPERATOR ------- */
int foo(int val) {
   if (val > 0) {
      goto positive_label;
   } else {
      return -1;
   }
positive_label:
   return val;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   int outer_var = 5;
   {
      int inner_var = 10;
      outer_var += inner_var;
   }
   return outer_var + val;
}
/* ------- SEPERATOR ------- */
enum Outer { A,
             B,
             C };

int foo() {
   enum Inner { A = 10,
                B = 20 };
   return A + B + C;
}
/* ------- SEPERATOR ------- */
int foo() {
   int x = 10;
   switch (x) {
      case 1:;
   }
   return 0;
}
/* ------- SEPERATOR ------- */
void func() {
   struct S {
      int x;
   } s;

   struct S s2 = {.x = 5};
}
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

char* invalid1 = "Hello, "
                 u8"World!";
/* ------- SEPERATOR ------- */
struct A {
   int x;
   struct B {
      int y;
   } b;
} globalA;

struct C {
   struct A a;
   struct B {
      int z;
   } b;
} globalC;
/* ------- SEPERATOR ------- */
struct A {
   struct {
      int x;
      struct {
         int y;
      };
   };
} globalA;
void foo() {
   int x = globalA.x;
   int y = globalA.y;
}
/* ------- SEPERATOR ------- */
int valid_int = 42;
int valid_function(void) {
   return 0;
}

int valid_test(void) {
   int a = valid_int;
   int b = valid_function();
   return a + b;
}
/* ------- SEPERATOR ------- */
enum Color1 { RED,
              GREEN,
              BLUE };
enum Color2 { YELLOW,
              RED,
              ORANGE };
int foo() {
   return RED;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   switch (val) {
      case 1:
         goto case_label;
      default:
         return -1;
      case_label:
         return val;
   }
}
/* ------- SEPERATOR ------- */
struct Complete {
   int value;
};

int test(void) {
   struct Complete arr[10];
   int index = 5;
   struct Complete value = arr[index];
   return value.value;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   int z = 70;
   {
      int z = 80;
      {
         int z = 90;
         val += z;
      }
      val += z;
   }
   val += z;
   return val;
}
/* ------- SEPERATOR ------- */
typeof(-3wb) minus_three_wb;
typeof(-0x3wb) minus_three_hex_wb;
typeof(3wb) three_wb;
typeof(3uwb) three_uwb;
typeof(-3uwb) minus_three_uwb;
/* ------- SEPERATOR ------- */
struct incomplete_type;

int test(void) {
   int x = 42;
   _Generic(x,
       int: "int",
       incomplete_type: "incomplete");
   return 0;
}
/* ------- SEPERATOR ------- */
int foo(int val) {
   int y = 40;
   {
      int y = 50;
      {
         int y = 60;
         return y;
      }
      return y;
   }
   return y;
}
/* ------- SEPERATOR ------- */
double valid_hex_double1 = 0x1.8p3;
double valid_hex_double2 = 0x1.8P-3;
float valid_hex_float1 = 0x1.8p3f;
float valid_hex_float2 = 0x1.8P-3f;
long double valid_hex_ldouble1 = 0x1.8p3L;
long double valid_hex_ldouble2 = 0x1.8P-3L;
/* ------- SEPERATOR ------- */
int test6(void) {
   volatile int x = 10;
   x++;
   return x;
}
/* ------- SEPARATOR ------- */
struct Point {
   int x;
   int y;
};
union Data {
   struct Point point;
   int x;
   int y;
};
int foo() {
   union Data data;
   return 0;
}
/* ------- SEPERATOR ------- */
_BitInt(2) a2 = 1;
_BitInt(3) a3 = 2;
_BitInt(33) a33 = 1;
signed char c = 3;
typeof(a2 * a3) a2a3;
typeof(a2 * c) a2c;
typeof(a33 * c) a33c;
/* ------- SEPERATOR ------- */
typeof(42) int_42;
typeof(2147483647) int_max;
typeof(2147483648) long_min;
typeof(4294967295) ulong_max;
typeof(9223372036854775807) ll_max;
typeof(18446744073709551615) ull_max;
typeof(0x2A) hex_42;
typeof(052) oct_42;
typeof(100000) int_large;
typeof(10000000000) ll_large;
typeof(-1) int_neg;
/* ------- SEPERATOR ------- */
_Decimal128 d128 = 1.0DD;
_Decimal64 d64 = 2.0DL;
_Decimal32 d32 = 3.0DF;
double dbl = 4.0;
long double ldbl = 5.0L;

typeof(d128 * d64) d128d64;
typeof(d128 * d32) d128d32;
typeof(d128 * dbl) d128dbl;
typeof(d128 * ldbl) d128ldbl;

typeof(d64 * d32) d64d32;
typeof(d64 * dbl) d64dbl;
typeof(d64 * ldbl) d64ldbl;

typeof(d32 * dbl) d32dbl;
typeof(d32 * ldbl) d32ldbl;
/* ------- SEPERATOR ------- */
#include <stddef.h>
#include <uchar.h>

unsigned char valid_oct = '\123';
unsigned char valid_hex = '\x7F';
char8_t valid_utf8 = u8'\x7F';
char16_t valid_utf16 = u'\x7FFF';
char32_t valid_utf32 = U'\x7FFFFFFF';
wchar_t valid_wchar = L'\x7FFF';
char* valid1 = "Hello, "
               "World!";
char* valid2 = u8"Hello, "
               u8"World!";
wchar_t* valid3 = L"Hello, "
                  L"World!";
char16_t* valid4 = u"Hello, "
                   u"World!";
char32_t* valid5 = U"Hello, "
                   U"World!"; // Adjacent UTF-32 string literals
