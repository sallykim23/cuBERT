#include "common_test.h"
#include <mpi.h>
TEST_F(CommonTest, memcpy) {
    MPI_Init(NULL,NULL);

    int rank;
    MPI_COMM_rank(MPI_COMM_WORLD, &rank);

    float *array = (float *) cuBERT::malloc(sizeof(float) * 10);
    float *array_b = (float *) cuBERT::malloc(sizeof(float) * 10);

    cuBERT::memcpy(array, array_b, sizeof(float) * 10, 3);

    cuBERT::free(array_b);
    cuBERT::free(array);

    MPI_Finalize();
}
