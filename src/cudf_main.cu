
#include "select/select.h"


using ral::execution::backend_id;
using ral::execution::execution_backend;

DEFINE_DISPATCH(select_stub);

void test_arrow_select(){
  auto backend = execution_backend(backend_id::ARROW);
//  backend_dispatcher(id, select_functor(), table_view, sortColIndices);
  select_stub(backend, 0, 5, 8);
}

 
int main() {
  test_arrow_select();
  return 0;
}
