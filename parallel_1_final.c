#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

int menu();

int main(int argc, char *argv[]) {
    int rank, size, n, i, selection;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    while (1) {

        int localSum = 0; // Reset localSum to zero at the beginning of each iteration
        int localMax = 0; // Initialize localMax to zero
       
       //Sending the menu choice to all processors         
        if (rank == 0) {
            selection = menu();
            for (int dest = 1; dest < size; dest++) {
                MPI_Send(&selection, 1, MPI_INT, dest, 3, MPI_COMM_WORLD);
            }
        } 
        else {
            MPI_Recv(&selection, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Synchronize all processes before proceeding
        MPI_Barrier(MPI_COMM_WORLD);

        // Check termination condition immediately after updating the selection variable
        if (selection != 1) {
            break;
        }

        if (rank == 0) {
            printf("Processor 0: Enter the total number of integers: ");
            scanf("%d", &n);
            int *allNumbers = (int *)malloc(n * sizeof(int));

            // Coordinator reads the integers in a loop
            printf("Processor 0: Enter %d integers:\n", n);
            for (i = 0; i < n; i++) {
                scanf("%d", &allNumbers[i]);
            }

            for (i = 0; i < n / size; i++) {
                localSum += allNumbers[i];
            }

            // Distribute the total number of integers to all processors without MPI_Bcast
            for (int dest = 1; dest < size; dest++) {
               MPI_Send(&n, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            }

            // Distribute the array of integers to all processors
            for (int dest = 1; dest < size; dest++) {
                MPI_Send(&allNumbers[dest * (n / size)], n / size, MPI_INT, dest, 1, MPI_COMM_WORLD);
            }

             // Coordinator (Processor 0) receives partial sums and calculates the overall sum
            int totalSum = 0;
            int totalMax = 0;
            double totalVar = 0.0; // Initialize totalVar to zero

            for (int source = 1; source < size; source++) {
                int partialSum;
                MPI_Recv(&partialSum, 1, MPI_INT, source, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                totalSum += partialSum;

                int partialMax;
                MPI_Recv(&partialMax, 1, MPI_INT, source, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (partialMax > totalMax) {
                    totalMax = partialMax;
                }
            }

            totalSum = totalSum + localSum;

            // Calculate processors's 0 local max and use it to find global max
            for (i = 0; i < n / size; i++) {
                if(allNumbers[i] > totalMax)
                    totalMax = allNumbers[i];
            }

            // Calculate and print the average
            double average = (double)totalSum / n;
            printf("The average is %.2f\n", average);
            // Print the global maximum
            printf("The global maximum is %d\n", totalMax);

            // Send the average to other processors
            for (int dest = 1; dest < size; dest++) {
                MPI_Send(&average, 1, MPI_DOUBLE, dest, 4, MPI_COMM_WORLD);
            }

            MPI_Barrier(MPI_COMM_WORLD);

            //calculating localVar for processor 0
            double localVar = 0.0;
            double number;
            for (i = 0; i < n / size; i++) {
                number = allNumbers[i] - average;
                localVar += (number * number);
            }          
            totalVar = totalVar+localVar;

            for (int source = 1; source < size; source++) {
                // Receive localVar from other processors
                double partialVar;
                MPI_Recv(&partialVar, 1, MPI_DOUBLE, source, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                totalVar += partialVar;
            }
            totalVar = totalVar/n;
            printf("Variance = %f\n", totalVar);

            // Print the new array
            printf("New Array:\n");
            for (i = 0; i < n; i++) {
                printf("%.1f ", pow(abs(allNumbers[i] - totalMax),2));
            }
            printf("\n");

            free(allNumbers);

        } else {
            // Other processors receive the total number of integers without MPI_Bcast
            MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Other processors allocate memory to receive their portion of the integers
            int *localNumbers = (int *)malloc((n / size) * sizeof(int));

            // Other processors receive their portion of the integers without MPI_Bcast
            MPI_Recv(localNumbers, n / size, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Calculate partial sum on each processor
            localSum = 0;
            for (i = 0; i < n / size; i++) {
                localSum += localNumbers[i];
                if (localNumbers[i] > localMax) {
                    localMax = localNumbers[i];
                }
            }

            // Send partial sums to the coordinator
            MPI_Send(&localSum, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
            MPI_Send(&localMax, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);

            MPI_Barrier(MPI_COMM_WORLD);

            // Receive the average from processor 0
            double average;
            MPI_Recv(&average, 1, MPI_DOUBLE, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            double localVar = 0;
            double number;
            for (i = 0; i < n / size; i++) {
                number = localNumbers[i] - average;
                localVar = localVar + (number*number);
            }
            MPI_Send(&localVar, 1, MPI_DOUBLE, 0, 5, MPI_COMM_WORLD);

            free(localNumbers);
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}

int menu() {
    int choice;
    printf("Select a choice:\n");
    printf("1. Continue\n2. Exit\n");
    scanf("%d", &choice);
    return choice;
}
