#include <stdio.h> //"Standar input&output"
#include <stdlib.h> //For memory management & rand()
#include <mpi.h>    //For MPI functions

#define MAX_VALUE 10 // Maximum value for random integers in the matrix

// Function to allocate memory for a matrix (2D array)
int** create_matrix(int rows, int cols) {
    // Allocate array of pointers (one for each row)
    int** matrix = (int**)malloc(rows * sizeof(int*));
    
    // Allocate one contiguous block for all data
    int* data = (int*)malloc(rows * cols * sizeof(int));
    
    // For each row, allocate space for the columns
    for (int i = 0; i < rows; i++) {
        matrix[i] = data + i * cols;
    }
    
    return matrix;
}

// Function to fill a matrix with random integers
void fillMatrixWithRandoms(int size, int *matrix) {
  for (int i = 0; i < size; i++) {
    matrix[i] = rand() % MAX_VALUE;
  }
}

// Function to free the memory used by a matrix
void free_matrix(int** matrix, int rows) {
    // With contiguous allocation, all rows reside in a single malloc'd block.
    // We free the base pointer once; the original per-row loop is no longer needed.
    free(matrix[0]);  // Free each row
    free(matrix);  // Free the array of pointers
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

int main(int argc, char* argv[]) { // So, the SO doesn`t pass arg directly, it passes a count and an array of strings.
	
	int matrixSize, processAmount, rank, size;

    // Initialize MPI before parsing arguments so we can use rank for error messages
    MPI_Init(&argc, &argv); //Init MPI environment
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //Get the rank of the process
    MPI_Comm_size(MPI_COMM_WORLD, &size); //Get the total number of processes

    if (argc > 2 && atoi(argv[1]) && atoi(argv[2])) {
        matrixSize = atoi(argv[1]);
				processAmount = atoi(argv[2]);
    } else {
        if (rank == 0) {
            printf("Usage: %s <matrixSize> <processAmount>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

	// RowsBatchs sizes.
	unsigned long long int rowsBatch = matrixSize / processAmount;
    if (matrixSize % processAmount != 0) {
        if (rank == 0) {
            printf("Error: matrixSize must be divisible by the number of processes (%d)\n", processAmount);
        }
        MPI_Finalize();
        return 1;
    }

	// Allocate local buffers for all processes
	int *batchA = (int *)malloc(rowsBatch * matrixSize * sizeof(int));
	int *batchB = (int *)malloc(matrixSize * matrixSize * sizeof(int));
	int *resultC = (int *)calloc(rowsBatch * matrixSize, sizeof(int));

	// RANK 0: Initialize and distribute data
	int** A = NULL;
	int** B = NULL;
	int** C = NULL;

	if (rank == 0) {
		// Allocate memory for all three matrices on rank 0
		A = create_matrix(matrixSize, matrixSize);
		B = create_matrix(matrixSize, matrixSize); 
		C = create_matrix(matrixSize, matrixSize);  // Result matrix
		
		// Fill matrices A and B with random integers
		fillMatrixWithRandoms(matrixSize * matrixSize, A[0]);
		fillMatrixWithRandoms(matrixSize * matrixSize, B[0]);
	}

	double start_time = MPI_Wtime(); // Start timing

	// Collective communication: MPI_Scatter distributes A to all processes
	// Each process (including rank 0) receives its batch of rows
	MPI_Scatter(rank == 0 ? A[0] : NULL, matrixSize * rowsBatch, MPI_INT,
	            batchA, matrixSize * rowsBatch, MPI_INT, 0, MPI_COMM_WORLD);

	// Collective communication: MPI_Bcast sends full matrix B to all processes
	// On rank 0: B[0] is the source; on other ranks: batchB is the destination
	MPI_Bcast(rank == 0 ? B[0] : batchB, matrixSize * matrixSize, MPI_INT, 0, MPI_COMM_WORLD);

	// All processes compute their batch of matrix C
	for (int r = 0; r < rowsBatch; r++) {          // row in this batch
		for (int column = 0; column < matrixSize; column++) {     // column in result
			int sum = 0;
			for (int k = 0; k < matrixSize; k++) { // sum index
				sum += batchA[r * matrixSize + k] *
					   batchB[k * matrixSize + column];
			}
			resultC[r * matrixSize + column] = sum;
		}
	}

	// Collective communication: MPI_Gather collects all computed batches into rank 0's C matrix
	MPI_Gather(resultC, matrixSize * rowsBatch, MPI_INT,
	           rank == 0 ? C[0] : NULL, matrixSize * rowsBatch, MPI_INT, 0, MPI_COMM_WORLD);

	double end = MPI_Wtime();

	if (rank == 0) {
		/*// Print matrices before freeing memory
		print_matrix(C, matrixSize, matrixSize, 'C');
		print_matrix(A, matrixSize, matrixSize, 'A');
		print_matrix(B, matrixSize, matrixSize, 'B');*/

		// Clean up memory
		free_matrix(A, matrixSize);
		free_matrix(B, matrixSize);
		free_matrix(C, matrixSize);

        printf("%f,", end - start_time);
	}

	// All processes free their local buffers
	free(batchA);
	free(batchB);
	free(resultC);

	MPI_Finalize();
	return 0;
}
