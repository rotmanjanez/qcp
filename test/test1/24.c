// Edge Case 3: Prototype with array parameter
int foo11(int arr[10]);

int foo11(int arr[10]) {
    return arr[0] + sizeof(arr); // array to pointer decay
}