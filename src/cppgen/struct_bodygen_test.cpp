#include "struct_bodygen.hpp"
#include <doctest.h>

namespace lidl::cpp {
namespace {
TEST_CASE("struct bodygen works") {
    module& m = get_root_module().get_child("mod");

    structure s;

    struct_body_gen;
}
}
}