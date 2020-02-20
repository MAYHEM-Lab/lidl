#include "union_generated.hpp"
#include <iostream>

int main() {
		std::array<uint8_t, 128> buf;
		lidl::message_builder builder(buf);
		auto& p = lidl::create<procedures>(builder, 42);	
		auto& p_str = lidl::create<procedures>(builder, lidl::create_string(builder, "foo"));	
	  std::cout << p.index() << '\n';
	  std::cout << p_str.index() << '\n';
}
