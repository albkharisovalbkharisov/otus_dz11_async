#include <iostream>
#include <string.h>
#include "async.h"
#include "thread"
#include "vector"

std::vector<std::thread> v;

void feed(decltype(async::connect(3)) &h, const char *str)
{
#if 0
	async::receive(h, str, strlen(str));
#else
	v.emplace_back(async::receive, h, str, strlen(str));
#endif
}

int main(int, char *[]) {
	try {
//		auto h = async::connect(3);
		auto h2 = async::connect(2);
//		async::receive(h, "1\n", 1);
		
		for (int i = 0; i < 500 ; ++i) {
			feed(h2, "0\n1\n");
			feed(h2, "3\n2\n1\n");
			feed(h2, "3\n2\n1\n");
			feed(h2, "5\n6\n7\n8\n9\n0\n1\n2\n3\n4\n3\n2\n1\n2\n2345234523452345234\n0\n1\n3\n2\n1\n");
		}
#if 0
		async::receive(h2, "{\n1\n", 2);
//		async::receive(h, "2\n3\n4\n5\n6\n{\na\n", 15);
//		async::receive(h, "b\nc\nd\n}\n89\n", 11);
		async::receive(h2, "b9\nb10\nb11\n", 11);
//		async::receive(h, "b9\nb10\nb11\n", 11);
		async::receive(h2, "b9\nb10\nb11\n", 11);
//		async::receive(h, "b9\nb10\nb11\n", 11);
		async::receive(h2, "b9\nb10\nb11\n", 11);
//		async::receive(h, "b9\nb10\nb11\n", 11);
		async::receive(h2, "b9\nb10\nb11\n", 11);
//		async::receive(h, "b9\nb10\nb11\n", 11);
		async::receive(h2, "b9\nb10\nb11\n", 11);
//		async::receive(h, "b9\nb10\nb11\n", 11);
		async::receive(h2, "b9\nb10\nb11\n}\n", 13);
#endif	// 0
		for (auto &t : v) {
			t.join();
		}
//		async::disconnect(h);
		async::disconnect(h2);
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}
