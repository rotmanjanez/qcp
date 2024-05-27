int test(void) {
    int arr[10];
    _Generic(arr, 
             int*: "int pointer", // Valid: array to pointer conversion
             float: "float"
    );
    return 0;
}