#include <stdio.h> //"Standar input&output"
#include <stdlib.h> //For memory management & rand()

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

int main() {
    int rows, cols;
    
    printf("=== Matrix Multiplication Program ===\n");
    
    // Get dimensions for Matrix A
    printf("Enter dimensions for the matrixs:\n");
    printf("Number of rows: ");
    scanf("%d", &rows);
    printf("Number of columns: ");
    scanf("%d", &cols);
    
    // Allocate memory for all three matrices
    int** A = create_matrix(rows, cols);
    int** B = create_matrix(rows, cols);
    int** C = create_matrix(rows, cols);  // Result matrix
    
    // Get input values
    input_matrix(A, rows, cols, 'A');
    input_matrix(B, rows, cols, 'B');
   
  // Perform multiplication
    printf("\nMultiplying matrices...\n");
    if (multiply_matrices(A, rows, cols, B, rows, cols, C)) {
        // Display result
        printf("\nResult (Matrix C = A × B):\n");
        print_matrix(C, rows, cols, 'C');
    }
    
    // Clean up memory
    free_matrix(A, rows);
    free_matrix(B, rows);
    free_matrix(C, rows);
    
    return 0;
}
