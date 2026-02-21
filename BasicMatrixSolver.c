#include <stdio.h> //"Standar input&output"
#include <stdlib.h> //For memory management & rand()
#include <sys/resource.h> // Provides the getusage() fuction and the struct type, which holds resource usage static_assert
#include <sys/time.h> //Provides the strcut timeval, needed for struct rusage

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

int main(int argc, char* argv[]) { // So, the SO doesn`t pass arg directly, it passes a count and an array of strings.
    
  struct rusage start, end; //Here start will be a snapchot before the operation, end will be a snapchot after
  getrusage(RUSAGE_SELF, &start); //&start is a pointer to the usage struct where the info will be saved
  //RUSAGE_SELF is a flag to meassure current process itself

  int rows = atoi(argv[1]);  // converts "4" → 4
  int cols = atoi(argv[2]);  // converts "4" → 4

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

  getrusage(RUSAGE_SELF, &end); // Takes the second snapshot

  double user_time = (end.ru_utime.tv_sec  - start.ru_utime.tv_sec) +
                   (end.ru_utime.tv_usec - start.ru_utime.tv_usec) / 1e6;
  /* Calculates the elpased user CPU time between two getusage() snapshots by combining two parts 
    tv_sec → whole seconds
    tv_usec → remaining microseconds (the fractional part, 0–999999)
  btw, microseconds is divided by 1e6 to convert ir into a fraction second
  */

  printf("%.6f ",user_time);
  
  return 0;
}
