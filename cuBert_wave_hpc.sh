#!/bin/bash
module load GCC/12.3.0
module load OpenMPI/4.1.5
MKL_PATH=/WAVE/projects/ycho_lab/software/intel/oneapi/mkl/2023.2.0/
export OMP_NUM_THREADS=1
export MKL_NUM_THREADS=1
export CUBERT_NUM_CPU_MODELS=1
export LD_LIBRARY_PATH=${MKL_PATH}/lib/intel64/:${LD_LIBRARY_PATH}

