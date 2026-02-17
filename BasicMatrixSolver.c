#include <stdio.h>
#include <stdlib.h>

/*
 * Matrix Multiplication Program
 * This program multiplies two matrices A and B to produce matrix C
 * 
 * Matrix dimensions:
 * - Matrix A: rows_A x cols_A
 * - Matrix B: rows_B x cols_B
 * - Matrix C: rows_A x cols_B
 * 
 * IMPORTANT RULE: cols_A must equal rows_B for multiplication to work!
 */

// Function to allocate memory for a matrix (2D array)
double** create_matrix(int rows, int cols) {
    // Allocate array of pointers (one for each row)
    double** matrix = (double**)malloc(rows * sizeof(double*));
    
    // For each row, allocate space for the columns
    for (int i = 0; i < rows; i++) {
        matrix[i] = (double*)malloc(cols * sizeof(double));
    }
    
    return matrix;
}

// Function to free the memory used by a matrix
void free_matrix(double** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);  // Free each row
    }
    free(matrix);  // Free the array of pointers
}

// Function to fill a matrix with values from user input
void input_matrix(double** matrix, int rows, int cols, char name) {
    printf("\nEnter values for Matrix %c (%dx%d):\n", name, rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("Enter element [%d][%d]: ", i, j);
            scanf("%lf", &matrix[i][j]);
        }
    }
}

// Function to print a matrix
void print_matrix(double** matrix, int rows, int cols, char name) {
    printf("\nMatrix %c (%dx%d):\n", name, rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%8.2f ", matrix[i][j]);  // Print with 2 decimal places
        }
        printf("\n");
    }
}

// Function to multiply two matrices
// Returns 1 if successful, 0 if dimensions don't match
int multiply_matrices(double** A, int rows_A, int cols_A,
                      double** B, int rows_B, int cols_B,
                      double** C) {
    
    // Check if multiplication is possible
    if (cols_A != rows_B) {
        printf("\nERROR: Cannot multiply matrices!\n");
        printf("Columns of A (%d) must equal rows of B (%d)\n", cols_A, rows_B);
        return 0;
    }
    
    // THE MULTIPLICATION ALGORITHM
    // For each element C[i][j], we calculate:
    // C[i][j] = A[i][0]*B[0][j] + A[i][1]*B[1][j] + ... + A[i][k]*B[k][j]
    // where k goes from 0 to cols_A-1 (or rows_B-1, they're the same)
    
    for (int i = 0; i < rows_A; i++) {           // For each row of A
        for (int j = 0; j < cols_B; j++) {       // For each column of B
            
            C[i][j] = 0.0;  // Initialize the result element to 0
            
            // Calculate the dot product of row i of A and column j of B
            for (int k = 0; k < cols_A; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    
    return 1;  // Success
}

int main() {
    int rows_A, cols_A, rows_B, cols_B;
    
    printf("=== Matrix Multiplication Program ===\n");
    printf("This program multiplies Matrix A by Matrix B\n\n");
    
    // Get dimensions for Matrix A
    printf("Enter dimensions for Matrix A:\n");
    printf("Number of rows: ");
    scanf("%d", &rows_A);
    printf("Number of columns: ");
    scanf("%d", &cols_A);
    
    // Get dimensions for Matrix B
    printf("\nEnter dimensions for Matrix B:\n");
    printf("Number of rows: ");
    scanf("%d", &rows_B);
    printf("Number of columns: ");
    scanf("%d", &cols_B);
    
    // Check if multiplication is possible before allocating memory
    if (cols_A != rows_B) {
        printf("\nERROR: Matrix multiplication not possible!\n");
        printf("Columns of A (%d) must equal rows of B (%d)\n", cols_A, rows_B);
        return 1;
    }
    
    // Allocate memory for all three matrices
    double** A = create_matrix(rows_A, cols_A);
    double** B = create_matrix(rows_B, cols_B);
    double** C = create_matrix(rows_A, cols_B);  // Result matrix
    
    // Get input values
    input_matrix(A, rows_A, cols_A, 'A');
    input_matrix(B, rows_B, cols_B, 'B');
    
    // Display input matrices
    print_matrix(A, rows_A, cols_A, 'A');
    print_matrix(B, rows_B, cols_B, 'B');
    
    // Perform multiplication
    printf("\nMultiplying matrices...\n");
    if (multiply_matrices(A, rows_A, cols_A, B, rows_B, cols_B, C)) {
        // Display result
        printf("\nResult (Matrix C = A × B):\n");
        print_matrix(C, rows_A, cols_B, 'C');
    }
    
    // Clean up memory
    free_matrix(A, rows_A);
    free_matrix(B, rows_B);
    free_matrix(C, rows_A);
    
    printf("\nProgram finished successfully!\n");
    
    return 0;
}
