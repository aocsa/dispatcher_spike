
#include "compute/backend_register.h"
#include "select/select.h"
 
namespace cpu{
  void select_impl(int type, int64_t a, int64_t b) {
    std::cout << "SELECT: cpu : a+b " << a + b << std::endl;
  }
}

REGISTER_DISPATCH(select_stub, &cpu::select_impl)

 