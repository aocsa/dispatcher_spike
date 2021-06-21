
#include "select/select.h"

namespace ral{
namespace cache {

DEFINE_DISPATCH(select_stub);

void test_arrow_select(){
  auto backend = ral::execution::execution_backend(ral::execution::backend_id::ARROW);
//  backend_dispatcher(id, select_functor(), table_view, sortColIndices);
  select_stub(backend, 0, 5, 8);
}


}}
int main() {
  ral::cache::test_arrow_select();
  return 0;
}
