typeof(42) int_42;              // Should be of type int
typeof(2147483647) int_max;     // Should be of type int (maximum value for int)
typeof(2147483648) long_min;    // Should be of type long (exceeds maximum value for int)
typeof(4294967295) ulong_max;   // Should be of type unsigned long (fits in unsigned long but not in signed types)
typeof(9223372036854775807) ll_max; // Should be of type long long (maximum value for long long)
typeof(18446744073709551615) ull_max; // Should be of type unsigned long long (maximum value for unsigned long long)
typeof(0x2A) hex_42;            // Should be of type int (hexadecimal constant)
typeof(052) oct_42;             // Should be of type int (octal constant)
typeof(100000) int_large;       // Should be of type int
typeof(10000000000) ll_large;   // Should be of type long long (exceeds range of int and long)
typeof(-1) int_neg;             // Should be of type int (negative integer constant)