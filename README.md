**Backend Dispatcher POC** 

This POC implements a static dispatcher for CPU and dynamic dispatcher for CUDA or any hardware accelerated platform. So, that code related to compute cuda and cpu can be compiled independently and then generate `kernels_cpu.so` and `kernels_cuda.so`. Using this dispatcher the kernels_cuda.so lib can be loaded dynamically and then use where is needed.

*Installation*

- git clone this repo and go the folder `dispatcher_spike`
- `mkdir build && cd build`
- `cmake ..`
- `make all`

*Main points*
- The build generates `libkernels_cuda.so` and `libkernels_cpu.so`
- There are two binaries `cpu_main` (see cpu_main.cpp) and `cuda_cpu_main` (see cpu_cpu_main.cu). 
- `cpu_main` it uses only the static `cpu` dispatcher as it should be the default backend and it is only compiled with a simple c++ compiler  
- `cuda_cpu_main` it uses both the `cpu` and `cuda` dispatcher it uses the `nvvc` compiler so cuda enabled hardware is present besides cpu hardware.  
