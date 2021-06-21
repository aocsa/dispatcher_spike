
#include "compute/backend_register.h"
#include "select/select.h"

namespace ral {
namespace cache {

void cpu_select_impl(int type, int64_t a, int64_t b) {
  std::cout << "SELECT: cpu : a+b " << a + b << std::endl;
}

REGISTER_DISPATCH(select_stub, &ral::cache::cpu_select_impl)

}}
