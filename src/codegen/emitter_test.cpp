#include "emitter.hpp"

#include <doctest.h>
#include <lidl/module.hpp>

namespace lidl::codegen {
namespace {
TEST_CASE("emitter works") {
    auto mod = lidl::basic_module();
    section s;
    s.definition = "struct foo {};";
    s.keys.emplace_back("foo_def");

    section bar;
    bar.definition = "struct bar { foo f; };";
    bar.depends_on.emplace_back("foo_def");

    emitter e(*mod, *mod, std::move(sections{{std::move(bar), std::move(s)}}));

    REQUIRE_EQ("struct foo {};\nstruct bar { foo f; };\n", e.emit());
}
} // namespace
} // namespace lidl::codegen