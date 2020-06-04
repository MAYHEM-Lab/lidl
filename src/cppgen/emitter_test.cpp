#include "emitter.hpp"
#include <doctest.h>

namespace lidl::cpp {
namespace {
TEST_CASE("emitter works") {
    section s;
    s.name = "foo_def";
    s.definition = "struct foo {};";

    section bar;
    bar.name = "bar_def";
    bar.definition = "struct bar { foo f; };";
    bar.depends_on.emplace_back("foo_def");

    emitter e(std::move(sections{{std::move(bar), std::move(s)}}));

    REQUIRE_EQ("struct foo {};\nstruct bar { foo f; };\n", e.emit());
}
}
}