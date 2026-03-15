#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <pthread.h>

// --- Thread argument struct ---
// Each thread needs to know which rows to compute, and where the matrices are
typedef struct {
    int start_row;  // First row this thread is responsible for
    int end_row;    // One past the last row (exclusive upper bound)
    int cols_A;     // = cols of A = rows of B (needed for the dot product loop)
    int cols_B;     // = cols of B = cols of C
    int** A;
    int** B;
    int** C;
} ThreadArgs;

// --- Thread worker function ---
// Each thread computes C[i][j] for i in [start_row, end_row)
void* multiply_rows(void* arg) {
    ThreadArgs* t = (ThreadArgs*)arg;

    for (int i = t->start_row; i < t->end_row; i++) {
        for (int j = 0; j < t->cols_B; j++) {
            t->C[i][j] = 0;
            for (int k = 0; k < t->cols_A; k++) {
                t->C[i][j] += t->A[i][k] * t->B[k][j];
            }
        }
    }

    return NULL;
}

// Function to allocate memory for a matrix (2D array)
int** create_matrix(int rows, int cols) {
    int** matrix = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        matrix[i] = (int*)malloc(cols * sizeof(int));
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

// Function to fill a matrix with random values
void input_matrix(int** matrix, int rows, int cols, char name) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = rand() % 100;
        }
    }
}

// Function to print a matrix
void print_matrix(int** matrix, int rows, int cols, char name) {
    printf("\nMatrix %c (%dx%d):\n", name, rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

// --- Threaded matrix multiplication ---
// Spawns num_threads threads, each handling a contiguous chunk of rows of C
void multiply_matrices_threaded(int** A, int rows_A, int cols_A,
                                int** B, int cols_B,
                                int** C, int num_threads) {
    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];

    // Calculate how many rows each thread gets
    // Using integer division; the last thread absorbs any remainder
    int rows_per_thread = rows_A / num_threads;

    for (int t = 0; t < num_threads; t++) {
        args[t].start_row = t * rows_per_thread;
        // Last thread takes all remaining rows (handles rows_A % num_threads remainder)
        args[t].end_row   = (t == num_threads - 1) ? rows_A : (t + 1) * rows_per_thread;
        args[t].cols_A    = cols_A;
        args[t].cols_B    = cols_B;
        args[t].A         = A;
        args[t].B         = B;
        args[t].C         = C;

        pthread_create(&threads[t], NULL, multiply_rows, &args[t]);
    }

    // Wait for all threads to finish before returning
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
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

    multiply_matrices_threaded(A, 3, 3, B, 3, C, 4);

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
    int num_threads = atoi(argv[2]);
    
    int cols = rows;

    int** A = create_matrix(rows, cols);
    int** B = create_matrix(rows, cols);
    int** C = create_matrix(rows, cols);

    input_matrix(A, rows, cols, 'A');
    input_matrix(B, rows, cols, 'B');

    // Perform threaded multiplication
    multiply_matrices_threaded(A, rows, cols, B, cols, C, num_threads);

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
