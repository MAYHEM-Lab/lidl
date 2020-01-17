#include <proto/basic.hpp>
#include <string_view>
#include <unordered_map>

namespace proto { 
namespace {
std::unordered_map<std::string_view, type*> basic_type_cache;
}

void init_basic_types() {
	auto int8_type = new integral_type;
	int8_type->size_in_bits = 8;
	basic_type_cache.emplace("i8", int8_type);
}

const type* basic_type(std::string_view name) {
	auto it = basic_type_cache.find(name);
	if (it == basic_type_cache.end()) {
		return nullptr;
	}
	return it->second;
}
}
