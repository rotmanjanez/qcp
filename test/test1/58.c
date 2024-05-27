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