add_library(lidl_eval
    include/lidl/eval.hpp
    include/lidl/eval/expression.hpp
    include/lidl/eval/literals.hpp
    eval.cpp
        expression.cpp literals.cpp include/lidl/eval/detail/expression_common.hpp include/lidl/eval/evaluation.hpp evaluation.cpp include/lidl/eval/value.hpp include/lidl/eval/relational_expressions.hpp relational_expressions.cpp value.cpp)
target_compile_features(lidl_eval PUBLIC cxx_std_20)
target_link_libraries(lidl_eval PUBLIC lidl_core)
target_include_directories(lidl_eval PUBLIC include)