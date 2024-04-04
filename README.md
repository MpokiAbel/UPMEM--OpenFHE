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
## Rules on adding dpu program

1. Make sure the program is placed in the folder /src/core/Pim/dpu
2. The dpu program name should end with an extension *_dpu.c example add_dpu.c
