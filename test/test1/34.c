// Example 1: Basic use of label
int foo1(int val) {
  label1: return val;
	// unreachable
  goto label1;
}

// Example 2: Label with a loop
int foo2(int val) {
  int i = 0;
  start_loop:
    if (i >= val) goto end_loop;
    i++;
    goto start_loop;
  end_loop: return i;
}

// Example 4: Jumping forward to a label
int foo4(int val) {
  goto forward_label;
  forward_label: return val * 2;
}

// Example 5: Label in a switch-case
int foo5(int val) {
  switch (val) {
    case 1:
      goto case_label;
    default:
      return -1;
    case_label:
      return val;
  }
}

// Example 6: Label in an if-else structure
int foo6(int val) {
  if (val > 0) {
    goto positive_label;
  } else {
    return -1;
  }
  positive_label:
    return val;
}

// Example 7: Label before a declaration (valid in C99 and onwards) todo: is that so?
int foo7(int val) {
  goto skip_init;
  int x = val;
  skip_init:
    return x; // May be uninitialized if val is skipped
}

// Example 8: Multiple labels pointing to the same statement
int foo8(int val) {
  if (val == 1) goto label;
  if (val == 2) goto label;
  return -1;
  label: return val;
}

// Example 9: Using a label across different blocks
int foo9(int val) {
  {
    goto out_of_block;
  }
  out_of_block: return val;
}

// Example 10: Label at the end of the function
int foo10(int val) {
  if (val < 0) goto end_function;
  return val * 2;
  end_function: return 0;
}