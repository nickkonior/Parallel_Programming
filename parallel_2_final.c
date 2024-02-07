#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

void readMatrix(int *matrix, int N);
void readMatrixFromFile(int *matrix, const char* filename);
void distributeMatrix(int *matrix, int *localMatrix, int rank, int size, int N);
bool checkDiagonalDominance();
bool isMatrixDiagonallyDominant(int localResult, int size);
bool checkLocalDiagonalDominance(int *localMatrix, int localRows, int globalRowStartIndex, int N);
int computeMaxAbsDiagonal(int *localMatrix, int localRows, int globalRowStartIndex, int size, int rank, int N);
void createMatrixB(int *localMatrix, int *localMatrixB, int localRows, int globalRowStartIndex, int globalMax, int N);
void printLocalMatrix();
int checkLocalMinPos(int *localMatrixB, int localRows, int globalRowStartIndex, int globalMax, int N);
void printMatrix(int *matrix, int N);

int main(int argc, char **argv) {
    int rank, size, N, globalMinPos;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        printf("Enter the size of the matrix N: ");
        scanf("%d", &N);
    }
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int localMatrixSize = (N * N) / size;
    int *matrix = malloc(N * N * sizeof(int));
    int *localMatrix = malloc(localMatrixSize * sizeof(int));
    int *localMatrixB = malloc(localMatrixSize * sizeof(int));
    int localRows = N / size, globalRowStartIndex = rank * localRows;

    if (rank == 0) {
        readMatrix(matrix, N);
        //printMatrix(matrix, N);
    }

    // Distribute the matrix to all processors
    distributeMatrix(matrix, localMatrix, rank, size, N);
    // Each processor checks diagonal dominance in its local matrix
    bool globalDominance = isMatrixDiagonallyDominant(checkLocalDiagonalDominance(localMatrix, localRows, globalRowStartIndex, N), size);
    
    // Processor 0 prints the result of diagonal dominance check
    if (rank == 0) {
        printf(globalDominance ? "yes\n" : "no\n");
    }

    // Create matrix B based on matrix A and global maximum value
    int globalMax = computeMaxAbsDiagonal(localMatrix, localRows, globalRowStartIndex, size, rank, N);
    createMatrixB(localMatrix, localMatrixB, localRows, globalRowStartIndex, globalMax, N);

    int *fullMatrixB = rank == 0 ? malloc(N * N * sizeof(int)) : NULL;
    MPI_Gather(localMatrixB, localMatrixSize, MPI_INT, fullMatrixB, localMatrixSize, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Full matrix B:\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                printf("%d ", fullMatrixB[i * N + j]);
            }
            printf("\n");
        }
    }

    int localMin = INT_MAX;
    for (int i = 0; i < localRows; i++) {
        for (int j = 0; j < N; j++) {
            if (localMatrixB[i * N + j] < localMin) {
                localMin = localMatrixB[i * N + j];
            }
        }
    }
    // Find and print the global minimum value and its position
    int globalMin;
    MPI_Allreduce(&localMin, &globalMin, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    // Calculate the local minimum position
    int localMinPos = checkLocalMinPos(localMatrixB, localRows, globalRowStartIndex, globalMax, N);
    // Use MPI_Allreduce to find the global minimum position
    MPI_Allreduce(&localMinPos, &globalMinPos, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    if (rank == 0) {
        printf("Global minimum value is %d at position (%d, %d)\n", globalMin, globalMinPos / N, globalMinPos % N);
    }

    // Clean up and finalize MPI
    MPI_Finalize();
    free(matrix);
    free(localMatrix);
    free(localMatrixB);
    free(fullMatrixB);
    return 0;
}

//Reads a matrix of size NxN from user input. 
//Matrix is a pointer to the first element of the matrix, and N is the size of the matrix.
void readMatrix(int *matrix, int N) {
    printf("Enter the matrix (%d x %d) row by row:\n", N, N);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            scanf("%d", &matrix[i * N + j]);
        }
    }
}

//Distributes the matrix evenly among MPI processes
//It uses MPI_Scatter to send portions of the main matrix to each process's localMatrix
void distributeMatrix(int *matrix, int *localMatrix, int rank, int size, int N) {
    int rowCount = N / size;  // Number of rows per processor
    MPI_Scatter(matrix, rowCount * N, MPI_INT, localMatrix, rowCount * N, MPI_INT, 0, MPI_COMM_WORLD);
}

//Checks if the local portion of the matrix is diagonally dominant.
//It returns true if each row's diagonal element is greater than the sum of the absolute values of other elements in the row.
bool checkLocalDiagonalDominance(int *localMatrix, int localRows, int globalRowStartIndex, int N) {
    for (int i = 0; i < localRows; i++) {
        int diagIndex = globalRowStartIndex + i;
        int rowSum = 0;
        int diagValue = 0;

        for (int j = 0; j < N; j++) {
            int value = localMatrix[i * N + j];
            if (j == diagIndex) {
                diagValue = abs(value);
            } else {
                rowSum += abs(value);
            }
        }
        if (diagValue <= rowSum) {
            return false; // The local part is not diagonally dominant
        }
    }
    return true; // The local part is diagonally dominant
}

//Collectively checks if the entire matrix is diagonally dominant across all MPI processes using MPI_Allreduce.
bool isMatrixDiagonallyDominant(int localResult, int size) {
    bool globalResult;
    MPI_Allreduce(&localResult, &globalResult, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);
    return globalResult;
}

//Computes the maximum absolute value of the diagonal elements in the local matrix
//and then finds the global maximum using MPI_Allreduce
int computeMaxAbsDiagonal(int *localMatrix, int localRows, int globalRowStartIndex, int size, int rank, int N) {
    int localMax = 0;
    for (int i = 0; i < localRows; i++) {
        int diagIndex = globalRowStartIndex + i;
        int diagValue = abs(localMatrix[i * N + diagIndex]);
        if (diagValue > localMax) {
            localMax = diagValue;
        }
    }
    int globalMax;
    MPI_Allreduce(&localMax, &globalMax, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Global maximum absolute diagonal value is: %d\n", globalMax);
    }
    return globalMax;
}

//Creates a local portion of matrix B based on the local portion of matrix A and the global maximum absolute diagonal value.
void createMatrixB(int *localMatrix, int localMatrixB[], int localRows, int globalRowStartIndex, int globalMax, int N) {
    for (int i = 0; i < localRows; i++) {
        for (int j = 0; j < N; j++) {
            if ((globalRowStartIndex + i) == j) {
                localMatrixB[i * N + j] = globalMax;
            } else {
                localMatrixB[i * N + j] = globalMax - abs(localMatrix[i * N + j]);
            }
        }
    }   
}

//Prints the local portion of a matrix
//This is used to display the part of the matrix that each MPI process is working on.
void printLocalMatrix(int *localMatrix, int localRows, int N) {
    printf("Local matrix portion:\n");
    for (int i = 0; i < localRows; i++) {
        for (int j = 0; j < N; j++) {
            printf("%d ", localMatrix[i * N + j]);
        }
        printf("\n");
    }
}

//Prints the full matrix. Typically used by process 0 to display the entire matrix
void printMatrix(int *matrix, int N) {
    printf("Full matrix read by processor 0:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%d ", matrix[i * N + j]);
        }
        printf("\n");
    }
}

//Determines the position of the local minimum value in the local portion of matrix B.
//This position is calculated relative to the entire matrix.
int checkLocalMinPos(int *localMatrixB, int localRows, int globalRowStartIndex, int globalMax, int N) {
    int localMin = INT_MAX;
    int localMinPos = -1;
    for (int i = 0; i < localRows; i++) {
        for (int j = 0; j < N; j++) {
            if (localMatrixB[i * N + j] < localMin) {
                localMin = localMatrixB[i * N + j];
                localMinPos = (globalRowStartIndex + i) * N + j;
            }
        }
    }
    return localMinPos;
}
