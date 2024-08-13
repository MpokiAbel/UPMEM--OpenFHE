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

Run : 
```
./bin/benchmark/upmem-poly-benchmark.
```
The benchmark has some precofigure polynomial parameter for polynomial generations using the openFHE APIs

You can manually reconfigure and recompile the following variables to your needs:

* `tow_args`: signifies the count of towers for each created polynomial set to 2, 4, and 8 by default.  
* `dpuNo`: denotes the quantity of DPUs, with a default of 256; ensure the DPU count follows a logarithmic relation with the base 2.  
* `RING_DIM_LOG`: indicates the ring dimension of the polynomial, defaulting to a size of 14.  
* `POLY_NUM`: signifies the quantity of polynomials necessary for the aforementioned configurations, typically 2 for each ciphertext.

## UPMEM PIM Integration

Currently, for experimentation purposes, we have not implemented a full backend based on PIM. Therefore, we only intercept certain HE operations and offload them to PIM. A more effective approach would be to integrate PIM as one of the backends of openFHE and re-implement a complete HAL layer with PIM, akin to [intel-hexl](https://github.com/intel/hexl).

The majority of PIM-based implementations reside in the folders `/src/core/pim` and `/src/core/include/pim`. The host folder contains code for the host CPU, while the DPU folder houses the DPU kernels.

### Rules on adding dpu kernels

1. Ensure the program is located in the directory `/src/core/pim/dpu`
2. The program for dpu should be named with the extension `*_dpu.c`, for instance `add_dpu.c`
