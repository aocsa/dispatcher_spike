
#include "compute/backend_register.h"
#include "select/select.h"
 
namespace cuda{
  void select_impl(int type, int64_t a, int64_t b) {
    std::cout << "SELECT: cuda : a+b " << a + b << std::endl;
  }
}

REGISTER_DISPATCH(select_stub, &cuda::select_impl)
// static RegisterCUDADispatch<decltype(&cuda_select_impl), struct select_stub> select_stub__register(select_stub, &cuda_select_impl);