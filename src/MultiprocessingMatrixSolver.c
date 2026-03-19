#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>

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

// --- Forked matrix multiplication ---
void multiply_matrices_forked(int** A, int rows_A, int cols_A,
                               int** B, int cols_B,
                               int** C, int num_processes) {
    if (num_processes > rows_A) num_processes = rows_A;

    int rows_per_proc = rows_A / num_processes;

    for (int p = 0; p < num_processes; p++) {
        int start_row = p * rows_per_proc;
        int end_row   = (p == num_processes - 1) ? rows_A : (p + 1) * rows_per_proc;

        pid_t pid = fork();
        if (pid < 0) { perror("fork failed"); exit(1); }

        if (pid == 0) {
            // --- Child process ---
            for (int i = start_row; i < end_row; i++) {
                for (int j = 0; j < cols_B; j++) {
                    C[i][j] = 0;
                    for (int k = 0; k < cols_A; k++) {
                        C[i][j] += A[i][k] * B[k][j];
                    }
                }
            }
            exit(0);
        }
    }

    for (int p = 0; p < num_processes; p++) {
        wait(NULL);
    }
}

void test_3x3() {
    int rows = 3, cols = 3;

    int a[3][3] = { {1, 2, 3}, {4, 5, 6}, {7, 8, 9} };
    int b[3][3] = { {9, 8, 7}, {6, 5, 4}, {3, 2, 1} };

    int** A = create_matrix(3, 3);
    int** B = create_matrix(3, 3);

    int* C_data = mmap(NULL,
                       rows * cols * sizeof(int),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS,
                       -1, 0);
    if (C_data == MAP_FAILED) { perror("mmap failed"); exit(1); }

    int** C = (int**)malloc(rows * sizeof(int*));
    if (!C) { perror("malloc failed (C row pointers)"); exit(1); }
    for (int i = 0; i < rows; i++) C[i] = C_data + i * cols;

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            A[i][j] = a[i][j];
            B[i][j] = b[i][j];
        }

    multiply_matrices_forked(A, 3, 3, B, 3, C, 4);
    print_matrix(C, 3, 3, 'C');

    free_matrix(A, 3);
    free_matrix(B, 3);
    free(C);
    munmap(C_data, rows * cols * sizeof(int));
}

int main(int argc, char* argv[]) {

    // test_3x3();

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int rows          = atoi(argv[1]);
    int num_processes = atoi(argv[2]);
    int cols          = rows;

    int** A = create_matrix(rows, cols);
    int** B = create_matrix(rows, cols);

    int* C_data = mmap(NULL,
                       rows * cols * sizeof(int),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS,
                       -1, 0);
    if (C_data == MAP_FAILED) { perror("mmap failed"); exit(1); }

    int** C = (int**)malloc(rows * sizeof(int*));
    if (!C) { perror("malloc failed (C row pointers)"); exit(1); }
    for (int i = 0; i < rows; i++) C[i] = C_data + i * cols;

    input_matrix(A, rows, cols);
    input_matrix(B, rows, cols);

    multiply_matrices_forked(A, rows, cols, B, cols, C, num_processes);

    // print_matrix(C, rows, cols, 'C');

    free_matrix(A, rows);
    free_matrix(B, rows);
    free(C);
    munmap(C_data, rows * cols * sizeof(int));

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec  - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%.6f,", elapsed);

    return 0;
}