#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>      // fork(), exit()
#include <sys/wait.h>    // wait()
#include <sys/mman.h>    // mmap(), munmap()

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

// Function to fill a matrix with values from user input
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

// --- Forked matrix multiplication ---
// Spawns num_processes children, each handling a contiguous chunk of rows of C.
// C must point into a shared memory region (mmap MAP_SHARED) so child writes
// are visible to the parent after the children exit.
void multiply_matrices_forked(int** A, int rows_A, int cols_A,
                               int** B, int cols_B,
                               int** C, int num_processes) {
    int rows_per_proc = rows_A / num_processes;

    for (int p = 0; p < num_processes; p++) {
        int start_row = p * rows_per_proc;
        // Last process absorbs any remainder rows (mirrors threaded version)
        int end_row = (p == num_processes - 1) ? rows_A : (p + 1) * rows_per_proc;

        pid_t pid = fork();

        if (pid == 0) {
            // --- Child process ---
            // Computes C[i][j] for i in [start_row, end_row)
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
        // Parent continues to spawn the next child immediately
    }

    // Parent waits for all children to finish before returning
    for (int p = 0; p < num_processes; p++) {
        wait(NULL);
    }
}


void test_3x3() { // This fuction allows me to test if the current implementation does multiplication of a specific matrix correctly
    // Known input matrices
    int rows = 3;
    int cols = 3;

    int a[3][3] = { {1, 2, 3},
                    {4, 5, 6},
                    {7, 8, 9} };

    int b[3][3] = { {9, 8, 7},
                    {6, 5, 4},
                    {3, 2, 1} };

    // Build int** wrappers so create/free/multiply functions work as-is
    int** A = create_matrix(3, 3);
    int** B = create_matrix(3, 3);

    // C must live in shared memory so that writes from child processes
    // are visible in the parent after they exit.
    // mmap with MAP_SHARED | MAP_ANONYMOUS gives all forked children
    // a view into the same physical pages.
    int* C_data = mmap(NULL,
                       rows * cols * sizeof(int),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS,
                       -1, 0);


    // Build the row-pointer array locally (each process gets its own copy
    // after fork, but every pointer still points into the shared C_data).
    int** C = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        C[i] = C_data + i * cols;
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            A[i][j] = a[i][j];
            B[i][j] = b[i][j];
        }
    }

    multiply_matrices_forked(A, 3, 3, B, 3, C, 4);

    print_matrix(C, 3, 3, 'C');

    free_matrix(A, 3);
    free_matrix(B, 3);
    free(C);
    munmap(C_data, rows * cols * sizeof(int));    // release shared memory
}


int main(int argc, char* argv[]) {

    //test_3x3();

    struct rusage start, end;
    getrusage(RUSAGE_SELF, &start);

    int rows = atoi(argv[1]);
    int num_processes = atoi(argv[2]);

    int cols = rows;

    int** A = create_matrix(rows, cols);
    int** B = create_matrix(rows, cols);

    // C must live in shared memory so that writes from child processes
    // are visible in the parent after they exit.
    // mmap with MAP_SHARED | MAP_ANONYMOUS gives all forked children
    // a view into the same physical pages.
    int* C_data = mmap(NULL,
                       rows * cols * sizeof(int),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS,
                       -1, 0);

    // Build the row-pointer array locally (each process gets its own copy
    // after fork, but every pointer still points into the shared C_data).
    int** C = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        C[i] = C_data + i * cols;
    }

    input_matrix(A, rows, cols, 'A');
    input_matrix(B, rows, cols, 'B');

    multiply_matrices_forked(A, rows, cols, B, cols, C, num_processes);

    //print_matrix(C, rows, cols, 'C');

    free_matrix(A, rows);
    free_matrix(B, rows);
    free(C);                                      // free row-pointer array
    munmap(C_data, rows * cols * sizeof(int));    // release shared memory

    getrusage(RUSAGE_SELF, &end);
    double user_time = (end.ru_utime.tv_sec  - start.ru_utime.tv_sec) +
                       (end.ru_utime.tv_usec - start.ru_utime.tv_usec) / 1e6;
    printf("%.6f,", user_time);

    return 0;
}
