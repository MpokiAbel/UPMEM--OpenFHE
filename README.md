OpenFHE - Open-Source Fully Homomorphic Encryption Library
=====================================

## Installation with UPMEM-PIM Project - Only on Linux platforms

Make sure that you have the [UPMEM TOOLCHAIN](https://sdk.upmem.com/stable/01_Install.html) installed !!

Run:  
```
mkdir -p build  
cd build  
cmake ..  
make -j4   
make build_dpu_programs  
```

To run the benchmark for the polynomial additions and multiplication after build.
run : ./bin/benchmark/upmem-poly-benchmark.

The benchmark has some precofigure polynomial parameter for polynomial generations using the openFHE APIs

You can manually reconfigure and recompile the following variables to your needs:

`tow_args`: represents the number of towers for each generated polynomial default 2, 4 and 8
`dpuNo` : represents the number of DPUs , the default value is 256: make sure the number of DPUs to be logn where log is to base 2;
`RING_DIM_LOG`: represents the ring dimension of the polynomial the default size is 14;
`POLY_NUM`: represents the number of polynomials needed for each setting above: usually the ciphertext has 2 polynomials so we leave it like that;

## UPMEM PIM Integration

Currently for purpose ofexperimentaion we have not implemented a full back-end based on PIM therefore we just intecept certain HE operations and offload to PIM. A much better approach is to have PIM as one of the backends of openFHE and re-implement a full HAL layer with PIM similar to [intel-hexl](https://github.com/intel/hexl)


Most of the Pim-based implementations are in a folder `/src/core/pim` and `/src/core/include/pim`. The host folder contains code for the host CPU and the DPU folder represents the DPU kernels.

### Rules on adding dpu kernels

1. Make sure the program is placed in the folder /src/core/pim/dpu
2. The dpu program name should end with an extension *_dpu.c example add_dpu.c
