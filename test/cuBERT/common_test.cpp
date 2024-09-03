#include "common_test.h"
#include <mpi.h>
TEST_F(CommonTest, memcpy) {
    MPI_Init(NULL,NULL);
    MPI_Barrier(MPI_COMM_WORLD);
    
    float *array = (float *) cuBERT::malloc(sizeof(float) * 10);
    float *array_b = (float *) cuBERT::malloc(sizeof(float) * 10);
    MPI_Barrier(MPI_COMM_WORLD);

    cuBERT::memcpy(array, array_b, sizeof(float) * 10, 3);
    cuBERT::free(array_b);
    cuBERT::free(array);

    MPI_Finalize();
}
