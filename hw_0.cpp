#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

int main(int argc, char **argv) {
    int n = atoi(argv[1]);

    MPI_Init(&argc, &argv);

    int size, rank, count;
    long long sum = 0, sum_0 = 0, number;
    MPI_Status status;

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    int process_size = n / (size - 1);

    if (rank == 0) {
        int* a = (int*)malloc(n * sizeof(int));
        for (int i = 0; i < n; ++i){
            a[i] = i;
            sum_0 += i;
        }
        for (int i = 1; i < size - 1; ++i) {
            MPI_Send(a + (i - 1) * process_size, process_size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        MPI_Send(a + (size - 2)*process_size, n%(size - 1) + process_size, MPI_INT, size - 1, 0, MPI_COMM_WORLD);
        free(a);
        std::cout << "Sum_0 = " << sum_0 << "\n";
    } else {
        MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
        MPI_Get_count( &status, MPI_INT, &count);
        int* a = (int*)malloc(count * sizeof(int));
        MPI_Recv(a, count, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        for (int i = 0; i < count; ++i){
            sum += a[i];
        }
        std::cout << "process " << rank << " received " << sum << "\n";
        free(a);
    }

    MPI_Barrier( MPI_COMM_WORLD );

    if (rank == 0) {
        for (int i = 1; i < size; ++i) {
            MPI_Recv(&number, 1, MPI_LONG_LONG, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            sum += number;
        }
        std::cout << "Sum = " << sum << "\n";
    } else {
        MPI_Send(&sum, 1, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
