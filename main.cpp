#include <iostream>
#include <string.h>
#include <thread>
#include <vector>
#include <sstream>
#include "async.h"

std::vector<std::thread> v;

void receive_from_thread(decltype(async::connect(3)) &h, const char *str)
{
#if 0
	async::receive(h, str, strlen(str));
#else
	v.emplace_back(async::receive, h, str, strlen(str));
#endif
}

int main(int, char *[]) {
	try {
#if 0
		async::handle_t h[4];
		int j = 2;
		for (auto &i : h) {
			i = async::connect(j++);
		}

		for (int i = 0; i < 500 ; ++i) {
			receive_from_thread(h[0], "0\n1\n2\n3\n4\n5\n6\n");
			receive_from_thread(h[1], "3\n2\n1\n0\n0\n0\n9\n");
			receive_from_thread(h[2], "3\n2\n1\n0\n0\n0\n0\n");
			receive_from_thread(h[3], "5\n6\n7\n8\n9\n0\n1\n2\n3\n4\n3\n2\n1\n2\n2345234523452345234\n0\n1\n3\n2\n1\n");
		}
		for (auto &t : v) {
			t.join();
		}
		for (auto i : h) {
			async::disconnect(i);
		}
#else
		async::handle_t h = async::connect(3);

		receive_from_thread(h,"0");
		receive_from_thread(h," ");
		receive_from_thread(h,"0");
		receive_from_thread(h,"\n");
		receive_from_thread(h,"\n");
		receive_from_thread(h,"1");
		receive_from_thread(h," ");
		receive_from_thread(h,"1");
		receive_from_thread(h,"\n");
		receive_from_thread(h,"2");
		receive_from_thread(h,"\n");


		for (auto &t : v) {
			t.join();
		}
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		async::disconnect(h);
#endif
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}

