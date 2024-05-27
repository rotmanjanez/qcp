struct Incomplete;

struct Incomplete test(struct Incomplete *arr) {
    int index = 5;
    return arr[index]; // Invalid: arr is a pointer to an incomplete type
}