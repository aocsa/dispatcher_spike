
#include "select/select.h"
#include <dlfcn.h>


using ral::execution::backend_id;
using ral::execution::execution_backend;


DEFINE_DISPATCH(select_stub);
void test_arrow_select(){
  auto backend = execution_backend(backend_id::ARROW);
  select_stub(backend, 0, 5, 8);
}



void test_cudf_select(){
  auto backend = execution_backend(backend_id::CUDF);

  void* handle = dlopen("./libcudf_dispatcher.so", RTLD_LAZY);
  if (!handle) {
      std::cerr << "dlopen error: " << dlerror() << '\n';
      return;
  }
  dlerror();
  select_stub(backend, 0, 5, 8);
  // close library
  dlclose(handle);
}


int main() {
  test_arrow_select();
  // TODO: link dynamically with libcudf_dispatcher.a
  test_cudf_select();
  return 0;
}
