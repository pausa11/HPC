#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <pthread.h>


// Function to allocate memory for a matrix (2D array)
int** create_matrix(int rows, int cols) {
    // Allocate array of pointers (one for each row)
    int** matrix = (int**)malloc(rows * sizeof(int*));
    
    // For each row, allocate space for the columns
    for (int i = 0; i < rows; i++) {
        matrix[i] = (int*)malloc(cols * sizeof(int));
    }
    
    return matrix;
}

// Function to free the memory used by a matrix
void free_matrix(int** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);  // Free each row
    }
    free(matrix);  // Free the array of pointers
}

// Function to fill a matrix with values from user input
void input_matrix(int** matrix, int rows, int cols, char name) {
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j]= rand() % 100;
        }
    }
}

// Function to print a matrix
void print_matrix(int** matrix, int rows, int cols, char name) {
    printf("\nMatrix %c (%dx%d):\n", name, rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);  // Print with 2 decimal places
        }
        printf("\n");
    }
}

int multiply_matrices(int** A, int rows_A, int cols_A,
                      int** B, int cols_B,          // B is already transposed
                      int** C) {
    for (int i = 0; i < rows_A; i++) {
        for (int j = 0; j < cols_B; j++) {
            int sum = 0;
            for (int k = 0; k < cols_A; k++) {
                sum += A[i][k] * B[j][k];           // Both accesses are row-sequential
            }
            C[i][j] = sum;
        }
    }
    return 1;
}

void test_3x3() { // This fuction allows me to test if the current implementation does multiplication of a specific matrix correctly
    // Known input matrices
    int a[3][3] = { {1, 2, 3},
                    {4, 5, 6},
                    {7, 8, 9} };

    int b[3][3] = { {9, 8, 7},
                    {6, 5, 4},
                    {3, 2, 1} };

    // Build int** wrappers so create/free/multiply functions work as-is
    int** A = create_matrix(3, 3);
    int** B = create_matrix(3, 3);
    int** C = create_matrix(3,3);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            A[i][j] = a[i][j];
            B[i][j] = b[i][j];
        }
    }

    multiply_matrices(A, 3, 3, B, 3, C);

    print_matrix(C, 3, 3, 'C');

    free_matrix(A, 3);
    free_matrix(B, 3);
    free_matrix(C,3);
}


int main(int argc, char* argv[]) {
    
    //test_3x3();

    struct rusage start, end;
    getrusage(RUSAGE_SELF, &start);

    int rows = atoi(argv[1]);
    int cols = rows;

    int** A = create_matrix(rows, cols);
    int** B = create_matrix(rows, cols);
    int** C = create_matrix(rows, cols);

    input_matrix(A, rows, cols, 'A');
    input_matrix(B, rows, cols, 'B');

    // Perform threaded multiplication
    multiply_matrices(A, rows, cols, B, cols, C);

    // print_matrix(C, rows, cols, 'C');

    free_matrix(A, rows);
    free_matrix(B, rows);
    free_matrix(C, rows);

    getrusage(RUSAGE_SELF, &end);
    double user_time = (end.ru_utime.tv_sec  - start.ru_utime.tv_sec) +
                       (end.ru_utime.tv_usec - start.ru_utime.tv_usec) / 1e6;

    printf("%.6f,", user_time);

    return 0;
}
