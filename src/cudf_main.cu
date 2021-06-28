
#include "select/select.h"


using ral::execution::backend_id;
using ral::execution::execution_backend;

DEFINE_DISPATCH(select_stub);

void test_arrow_select(){
  auto backend = execution_backend(backend_id::ARROW);
  select_stub(backend, 0, 5, 8);
}


void test_cudf_select(){
  auto backend = execution_backend(backend_id::CUDF);
  select_stub(backend, 0, 5, 8);
}


int main() {
  test_arrow_select();
  // TODO: link dynamically with libcudf_dispatcher.a
  test_cudf_select();
  return 0;
}
