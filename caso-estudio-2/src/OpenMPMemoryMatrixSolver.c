#include <stdio.h>
#include <stdlib.h>
#include <time.h> // Provides clock_gettime() for wall-clock measurement
#include <omp.h>

// Function to allocate memory for a matrix (2D array)
int** create_matrix(int rows, int cols) {
    int** matrix = (int**)malloc(rows * sizeof(int*));
    if (!matrix) { perror("malloc failed (matrix rows)"); exit(1); }
    for (int i = 0; i < rows; i++) {
        matrix[i] = (int*)malloc(cols * sizeof(int));
        if (!matrix[i]) { perror("malloc failed (matrix col)"); exit(1); }
    }
    return matrix;
}

// Function to free the memory used by a matrix
void free_matrix(int** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

void input_matrix(int** matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = rand() % 100;
        }
    }
}

void print_matrix(int** matrix, int rows, int cols, char name) {
    printf("\nMatrix %c (%dx%d):\n", name, rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

// --- OpenMP + cache-friendly matrix multiplication ---
// B is accessed as B[j][k] (row-sequential for both A and B), assuming the caller
// stores B such that this access pattern equals A * B (i.e., B is pre-transposed
// or treated as transposed in the access pattern, mirroring MemoryMatrixSolver.c).
void multiply_matrices_omp_mem(int** A, int rows_A, int cols_A,
                               int** B, int cols_B,
                               int** C, int num_threads) {
    omp_set_num_threads(num_threads);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows_A; i++) {
        for (int j = 0; j < cols_B; j++) {
            int sum = 0;
            for (int k = 0; k < cols_A; k++) {
                sum += A[i][k] * B[j][k];   // Both accesses are row-sequential
            }
            C[i][j] = sum;
        }
    }
}

void test_3x3() {
    int a[3][3] = { {1, 2, 3}, {4, 5, 6}, {7, 8, 9} };
    int b[3][3] = { {9, 8, 7}, {6, 5, 4}, {3, 2, 1} };

    int** A = create_matrix(3, 3);
    int** B = create_matrix(3, 3);
    int** C = create_matrix(3, 3);

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            A[i][j] = a[i][j];
            B[i][j] = b[i][j];
        }

    multiply_matrices_omp_mem(A, 3, 3, B, 3, C, 4);
    print_matrix(C, 3, 3, 'C');

    free_matrix(A, 3);
    free_matrix(B, 3);
    free_matrix(C, 3);
}

int main(int argc, char* argv[]) {

    // test_3x3();

    struct timespec start, end; // Wall-clock snapshots
    clock_gettime(CLOCK_MONOTONIC, &start);

    int rows        = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int cols        = rows;

    int** A = create_matrix(rows, cols);
    int** B = create_matrix(rows, cols);
    int** C = create_matrix(rows, cols);

    input_matrix(A, rows, cols);
    input_matrix(B, rows, cols);

    multiply_matrices_omp_mem(A, rows, cols, B, cols, C, num_threads);

    // print_matrix(C, rows, cols, 'C');

    free_matrix(A, rows);
    free_matrix(B, rows);
    free_matrix(C, rows);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec  - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("%.6f,", elapsed);

    return 0;
}
