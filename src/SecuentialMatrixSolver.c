#include <stdio.h> //"Standar input&output"
#include <stdlib.h> //For memory management & rand()
#include <time.h> // Provides clock_gettime() for wall-clock measurement

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

// Function to multiply two matrices
// Returns 1 if successful, 0 if dimensions don't match
int multiply_matrices(int** A, int rows_A, int cols_A,
                    int** B, int rows_B, int cols_B,
                    int** C) {
   
    for (int i = 0; i < rows_A; i++) {      // Outer loop iterate through each row of A
        for (int j = 0; j < cols_B; j++) {  // Middle loop iterate through each column of B 
            
            C[i][j] = 0;  // Initialize the result element to 0
            
            // Calculate the dot product of row i of A and column j of B
            for (int k = 0; k < cols_A; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    
    return 1;  // Success
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

    multiply_matrices(A, 3, 3, B, 3, 3, C);

    print_matrix(C, 3, 3, 'C');

    free_matrix(A, 3);
    free_matrix(B, 3);
    free_matrix(C,3);
}

int main(int argc, char* argv[]) { // So, the SO doesn`t pass arg directly, it passes a count and an array of strings.
  
  //test_3x3(); //to test if the mulitplication is done correctly

  struct timespec start, end; // Wall-clock snapshots
  clock_gettime(CLOCK_MONOTONIC, &start);

  int rows = atoi(argv[1]);  // converts "4" → 4
  int cols = rows;

  // Allocate memory for all three matrices
  int** A = create_matrix(rows, cols);
  int** B = create_matrix(rows, cols); 
  int** C = create_matrix(rows, cols);  // Result matrix
    
  // Get input values
  input_matrix(A, rows, cols, 'A');
  input_matrix(B, rows, cols, 'B');
   
  // Perform multiplication
  if (multiply_matrices(A, rows, cols, B, rows, cols, C)) {
      // Display result
      //printf("\nResult (Matrix C = A × B):\n");
      //print_matrix(C, rows, cols, 'C');
  }
    
  // Clean up memory
  free_matrix(A, rows);
  free_matrix(B, rows);
  free_matrix(C, rows);

  clock_gettime(CLOCK_MONOTONIC, &end);

  double elapsed = (end.tv_sec  - start.tv_sec) +
                   (end.tv_nsec - start.tv_nsec) / 1e9;

  printf("%.6f,", elapsed);
  
  return 0;
}
