#pragma once

#include <lidl/lidl.hpp>
#include <pybind11/pybind11.h>

namespace lidl::pybind {
enum class types : intptr_t
{
};

struct vector_base {
    virtual py::list get() const = 0;
    virtual ~vector_base() = default;
};

template<class T>
struct vec : vector_base {
    vec(lidl::vector<T>& v)
        : m_vec(&v) {
    }
    lidl::vector<T>* m_vec;
    py::list get() const override {
        py::list res;
        for (auto& val : *m_vec) {
            res.append(val);
        }
        return res;
    }
};
struct type_wrapper {
    virtual const std::string& name() = 0;
    virtual void add_creator(py::module&, py::class_<lidl::message_builder>&) = 0;
    virtual std::unique_ptr<vector_base> make_vector(lidl::message_builder&,
                                                     py::list) = 0;
    virtual ~type_wrapper() = default;
};

template<class T>
struct lidl_type_wrapper : type_wrapper {
    lidl_type_wrapper(std::string_view name)
        : m_name(std::string(name)) {
    }

    const std::string& name() override {
        return m_name;
    }

    void add_creator(py::module&, py::class_<lidl::message_builder>& c) override {
        c.def(
            "create",
            [](lidl::message_builder& builder, T val) -> T& {
                return lidl::emplace_raw<T>(builder, val);
            },
            py::return_value_policy::reference);
    }

    std::unique_ptr<vector_base> make_vector(lidl::message_builder& builder,
                                             py::list list) override {
        auto& v = lidl::create_vector<T>(builder, list[0].cast<T>());
        return std::make_unique<vec<T>>(v);
    }

    std::string m_name;
};

struct string_wrapper : type_wrapper {
    const std::string& name() override {
        return m_name;
    }

    void add_creator(py::module&, py::class_<lidl::message_builder>& c) override {
        c.def(
            "create",
            [](lidl::message_builder& builder, std::string val) -> lidl::string& {
                return lidl::create_string(builder, val);
            },
            py::return_value_policy::reference);
    }

    std::unique_ptr<vector_base> make_vector(lidl::message_builder& builder,
                                             py::list list) override {
        auto& v = lidl::create_vector<lidl::string>(
            builder, lidl::create_string(builder, list[0].cast<std::string>()));
        return std::make_unique<vec<lidl::ptr<lidl::string>>>(v);
    }

    std::string m_name = "string";
};


struct vector_wrapper : type_wrapper {
    const std::string& name() override {
        return m_name;
    }

    void add_creator(py::module&, py::class_<lidl::message_builder>& c) override {
        c.def(
            "create_vector",
            [](lidl::message_builder& builder,
               types idx,
               py::list val) -> std::unique_ptr<vector_base> {
                auto& wrapper = *wrapped_types.at(int(idx));
                return wrapper.make_vector(builder, std::move(val));
            },
            py::return_value_policy::reference);
    }

    std::unique_ptr<vector_base> make_vector(lidl::message_builder& builder,
                                             py::list list) override {
        throw std::runtime_error("can't make a vector-of-vectors");
    }

    std::string m_name = "vector";
};

template<class T>
struct struct_wrapper : type_wrapper {
    const std::string& name() override {
        return m_name;
    }

    void add_creator(py::module& m, py::class_<lidl::message_builder>& c) override {
        auto ctor_name = "create_" + m_name;
        py::class_<T> bind(m, name().c_str());
        if constexpr (!lidl::is_reference_type<T>{}) {
            add_raw_ctor(bind, typename lidl::struct_traits<T>::raw_members{});
        }
        m.def(ctor_name.c_str(),
              &lidl::struct_traits<T>::ctor,
              py::return_value_policy::reference);
        auto reg_getters = [&bind](const auto&... members) {
            (bind.def(members.name,
                      members.const_function,
                      py::return_value_policy::reference),
             ...);
        };
        std::apply(reg_getters, lidl::struct_traits<T>::members);
    }

    std::unique_ptr<vector_base> make_vector(lidl::message_builder& builder,
                                             py::list list) override {
        throw std::runtime_error("can't make a vector-of-vectors");
    }

private:
    template<class... Ts>
    void add_raw_ctor(py::class_<T>& c, lidl::meta::list<Ts...>) {
        c.def(py::init<const Ts&...>());
    }

    std::string m_name = lidl::struct_traits<T>::name;
};

template<class T>
struct union_wrapper : type_wrapper {
    const std::string& name() override {
        return m_name;
    }

    void add_creator(py::module& m, py::class_<lidl::message_builder>& c) override {
        auto ctor_name = "create_" + m_name;
        py::class_<T> bind(m, name().c_str());
        auto add_ctors = [&m, &ctor_name](const auto&... ctors) {
            (m.def(ctor_name.c_str(), ctors, py::return_value_policy::reference), ...);
        };
        std::apply(add_ctors, lidl::union_traits<T>::ctors);
        auto reg_getters = [&bind](const auto&... members) {
            (bind.def(members.name,
                      members.const_function,
                      py::return_value_policy::reference),
             ...);
        };
        bind.def("alternative", &T::alternative);
        std::apply(reg_getters, lidl::union_traits<T>::members);
    }

    std::unique_ptr<vector_base> make_vector(lidl::message_builder& builder,
                                             py::list list) override {
        throw std::runtime_error("can't make a vector-of-vectors");
    }

    std::string m_name = lidl::union_traits<T>::name;
};

template<class... Types, class... Names>
void create_basic_types(const Names&... names) {
    (wrapped_types.emplace_back(std::make_unique<lidl_type_wrapper<Types>>(names)), ...);
}

template<class T, std::enable_if_t<std::is_enum_v<T>>* = nullptr>
void bind(py::module& mod) {
    using traits = lidl::enum_traits<T>;
    py::enum_<T> bind(mod, traits::name);
    for (int i = 0; i < traits::names.size(); ++i) {
        bind.value(traits::names[i], T(i));
    }
}

template<class T, std::enable_if_t<lidl::is_struct<T>{}>* = nullptr>
void bind(py::module& mod) {
    wrapped_types.emplace_back(std::make_unique<struct_wrapper<T>>());
}

template<class T, std::enable_if_t<lidl::is_union<T>{}>* = nullptr>
void bind(py::module& mod) {
    wrapped_types.emplace_back(std::make_unique<union_wrapper<T>>());
}

template<class... Ts>
void bind_all(py::module& mod, lidl::meta::list<Ts...>) {
    (bind<Ts>(mod), ...);
}
} // namespace lidl::pybind