#include <stdio.h>
#include <stdlib.h>
#include <time.h> // Provides clock_gettime() for wall-clock measurement
#include <pthread.h>

// --- Thread argument struct ---
typedef struct {
    int start_row;
    int end_row;
    int cols_A;
    int cols_B;
    int** A;
    int** B;
    int** C;
} ThreadArgs;

// --- Thread worker function ---
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

// Function to allocate memory for a matrix
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

// FIX: removed unused 'name' parameter
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

// --- Threaded matrix multiplication ---
void multiply_matrices_threaded(int** A, int rows_A, int cols_A,
                                int** B, int cols_B,
                                int** C, int num_threads) {
    // FIX 1: cap threads to number of rows to avoid empty/redundant threads
    if (num_threads > rows_A) num_threads = rows_A;

    // FIX 2: heap-allocate instead of stack VLAs to avoid potential stack overflow
    pthread_t*  threads = malloc(num_threads * sizeof(pthread_t));
    ThreadArgs* args    = malloc(num_threads * sizeof(ThreadArgs));
    if (!threads || !args) { perror("malloc failed (thread arrays)"); exit(1); }

    int rows_per_thread = rows_A / num_threads;

    for (int t = 0; t < num_threads; t++) {
        args[t].start_row = t * rows_per_thread;
        args[t].end_row   = (t == num_threads - 1) ? rows_A : (t + 1) * rows_per_thread;
        args[t].cols_A    = cols_A;
        args[t].cols_B    = cols_B;
        args[t].A         = A;
        args[t].B         = B;
        args[t].C         = C;

        // FIX 3: check pthread_create return value
        int rc = pthread_create(&threads[t], NULL, multiply_rows, &args[t]);
        if (rc != 0) { fprintf(stderr, "pthread_create failed: %d\n", rc); exit(1); }
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    free(threads);
    free(args);
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

    // 4 threads for a 3-row matrix: safely capped to 3 by the fix
    multiply_matrices_threaded(A, 3, 3, B, 3, C, 4);
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

    multiply_matrices_threaded(A, rows, cols, B, cols, C, num_threads);

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