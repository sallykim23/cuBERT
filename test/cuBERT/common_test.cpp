#include "common_test.h"
#include <mpi.h>
extern "C" int my_Init(int *argc, char ***argv);
extern "C" int my_Barrier();
extern "C" int my_Wtime();
extern "C" int my_Finalize();

TEST_F(CommonTest, memcpy) {
    
    float *array = (float *) cuBERT::malloc(sizeof(float) * 10);
    float *array_b = (float *) cuBERT::malloc(sizeof(float) * 10);

    cuBERT::memcpy(array, array_b, sizeof(float) * 10, 3);
    
    cuBERT::free(array_b);
    cuBERT::free(array);
	
}
