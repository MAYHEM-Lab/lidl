#pragma once

#include <string>
#include <map>

namespace proto {
struct type {
	virtual ~type() = default;
};

struct integral_type : type {
	int32_t size_in_bits;
};

struct member {
   const type* type_;
   std::map<std::string, std::string> attributes;
};

struct structure {
	std::map<std::string, member> members;
};
}
