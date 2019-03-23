#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <fstream>
#include <ctime>
#include <string>
#include <shared_mutex>
#include <map>

#include <utility>
#include <memory>
#include <mutex>
#include "async.h"
#include "bulkmt.h"

namespace async {

static int handle_cnt = 0;
static std::map<async::handle_t, std::shared_ptr<bulk>> bm;
// smutex is mutex to protect access to bm.
// It is shared because we can feed 2 different bulks
// simultaniously. For protecting one certain bulk from
// 2 simultaious accesses there are mutex inside
// bulk class.
static std::shared_timed_mutex smutex;

async::handle_t connect(std::size_t bulk_size) {
	std::unique_lock<std::shared_timed_mutex> l{smutex};
	++handle_cnt;	// numeration starts from 1
	async::handle_t handle = reinterpret_cast<async::handle_t>(handle_cnt);
	bm.emplace(std::make_pair(handle, std::make_shared<bulk>(bulk_size)));
	return handle;
}

void receive(async::handle_t handle, const char *data, std::size_t size) {
	std::shared_lock<std::shared_timed_mutex> l{smutex};
	// no exception handling. Should be handled higher
	auto a = bm.at(handle);
	a->collect_and_parse(data, size);
}

void disconnect(async::handle_t handle) {
	std::unique_lock<std::shared_timed_mutex> l{smutex};
	bm.erase(handle);
}

}



