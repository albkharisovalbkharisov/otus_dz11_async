#include <iostream>

#include "async.h"

int main(int, char *[]) {
	try {
		std::size_t bulk = 2;
		auto h = async::connect(bulk);
//		auto h2 = async::connect(bulk);
		async::receive(h, "1\n", 1);
//		async::receive(h2, "1\n", 2);
		async::receive(h, "2\n3\n4\n5\n6\n{\na\n", 15);
		async::receive(h, "b\nc\nd\n}\n89\n", 11);
		async::receive(h, "b9\nb10\nb11\n", 11);
		async::receive(h, "b9\nb10\nb11\n", 11);
		async::receive(h, "b9\nb10\nb11\n", 11);
		async::disconnect(h);
//		async::disconnect(h2);
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}
