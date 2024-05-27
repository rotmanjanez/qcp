struct Complete {
    int value;
};

int test(void) {
    struct Complete arr[10];
    int index = 5;
    struct Complete value = arr[index]; // Valid: arr is "pointer to complete object type", index is integer type
    return value.value;
}