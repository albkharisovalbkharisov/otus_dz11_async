#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <fstream>
#include <ctime>
#include <string>
#include <signal.h>
#include <stdexcept>
#include <shared_mutex>
#include <map>

#include <utility>
#include <memory>
#include <chrono>
#include <condition_variable>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include "async.h"

template<bool count_lines_too = false>
class dbg_counter
{
	std::atomic<size_t> line_counter;
	std::atomic<size_t> cmd_counter;
	std::atomic<size_t> bulk_counter;
public:
	dbg_counter(void) : line_counter(0), cmd_counter(0), bulk_counter(0) {}
	dbg_counter(const dbg_counter &that)
	{
		this->line_counter.store(that.line_counter);
		this->cmd_counter.store(that.cmd_counter);
		this->bulk_counter.store(that.bulk_counter);
	}
	void line_inc(size_t i = 1) { line_counter += i; }
	void cmd_inc(size_t i = 1)  { cmd_counter  += i; }
	void bulk_inc(size_t i = 1) { bulk_counter += i; }

	void print_counters(const std::string &thread_name) const
	{
		std::cout << "thread " << thread_name << ": ";
		if (count_lines_too)
			std::cout << line_counter << " lines, ";
		std::cout << cmd_counter << " commands, ";
		std::cout << bulk_counter << " bulks" << std::endl;
	}
};

struct worker
{
	std::thread thread;
	std::string name;
	dbg_counter<false> dbg;
	worker(std::thread t, const char *s) : thread(std::move(t)), name(s) {}
	worker(const char *s) : name(s) {}
};

class IbaseClass
{
public:
	using type_to_handle = struct {
		const std::vector<std::string> vs;
		const std::time_t t;
	};
	std::condition_variable cv;
	std::mutex cv_m;
	std::queue<type_to_handle> qMsg;
	std::list<worker> vThread;
	std::atomic<bool> exit;
	virtual void handle(const type_to_handle &ht) { (void) ht; throw; }

	template<typename ...Names>
	IbaseClass(Names... names) : exit(false) {
		const char * dummy[] = {(const char*)(names)...};
		for (auto &s : dummy)
			vThread.emplace_back(s);
	}

	void start_threads(void)
	{
		for (auto &w : vThread)
			w.thread = std::thread(&IbaseClass::work, this, std::ref(w));   // moving thread is OK
	}

	void stop_threads(void)
	{
		exit = true;
		cv.notify_all();
		for (auto &a : vThread) {
			if (a.thread.joinable())
				a.thread.join();
			else
				std::cout << "stop_threads() : join..." << a.name << " can't!" << std::endl;
		}
	}

	virtual ~IbaseClass(void)
	{
		for (const auto &a : vThread)
			a.dbg.print_counters(a.name);
	}

	void notify(type_to_handle &ht)
	{
		{
			std::unique_lock<std::mutex> lk(cv_m);
			qMsg.push(ht);
		}
		cv.notify_one();
	}

private:
	void work(struct worker &w)
	{
		while(1) {
			std::unique_lock<std::mutex> lk(cv_m);
			cv.wait(lk, [this](){ return !this->qMsg.empty() || exit; });
			if (exit && qMsg.empty()) break;
			auto m = qMsg.front();
			qMsg.pop();
			lk.unlock();
			w.dbg.bulk_inc();
			w.dbg.cmd_inc(m.vs.size());
			this->handle(m);
		}
	}
};

using namespace std::chrono;
class saver : public IbaseClass
{
public:
	template<typename ...Names>
	saver(Names... names) : IbaseClass(names...) {}
	void handle(const type_to_handle &ht) override {

		// invent name
		std::hash<std::thread::id> hash_thread_id;
		size_t hash = hash_thread_id(std::this_thread::get_id()) ^ std::hash<int>()(std::rand());
		std::string filename = "bulk" + std::to_string(ht.t) + "_" + std::to_string(hash) + ".log";

		std::fstream fs;
		fs.open (filename, std::fstream::in | std::fstream::out | std::fstream::app);
		for (auto &a : ht.vs) {
			fs << a;
			fs << '\n';
		}
		fs.close();
	}
};

class printer : public IbaseClass
{
public:
	template<typename ...Names>
	printer(Names... names) : IbaseClass(names...) {}
	void handle(const type_to_handle &ht) override {
		std::cout << output_string_make(ht.vs);
	}

private:
	std::string output_string_make(const std::vector<std::string> &vs)
	{
		bool first = true;
		std::string s("bulk: ");
		for (const auto &si : vs) {
			if (!first)
				s += ", ";
			else
				first = false;
			s += si;
		}
		s += '\n';
		return s;
	}
};

class bulk : public dbg_counter<true>
{
	const size_t bulk_size;
	std::vector<std::string> vs;
	std::list<std::shared_ptr<IbaseClass>> lHandler;
	size_t brace_cnt;
	std::time_t time_first_chunk;
	std::mutex m;

public:
	bulk(size_t size) : bulk_size(size), brace_cnt(0), time_first_chunk(0) {
		vs.reserve(bulk_size);
		auto print_ptr = std::shared_ptr<IbaseClass> (new printer("log"));
		auto save_ptr = std::shared_ptr<IbaseClass> (new saver("file1", "file2"));
//		auto a = std::make_shared<printer>("log");
//		auto b = std::make_shared<saver>("file1", "file2");
		this->add_handler(print_ptr);
		this->add_handler(save_ptr);
		print_ptr->start_threads();
		save_ptr->start_threads();
	}

	void add_handler(std::shared_ptr<IbaseClass> &handler) {
		lHandler.push_back(handler);
	}

	void flush(void) {
		if (vs.size() == 0)
			return;

		bulk_inc();
		IbaseClass::type_to_handle ht = {vs, time_first_chunk};
		for (const auto &h : lHandler) {
			h->notify(ht);
		}
		vs.clear();
		time_first_chunk = 0;
	}

	void add(std::string &s);
	void signal_callback_handler(int signum);
	bool is_full(void);
	bool is_empty(void);
	void parse_line(std::string line);
	~bulk(){
		print_counters("main");
		for (const auto &h : lHandler) {
			h->stop_threads();
		}
	}
};


void bulk::parse_line(std::string line)
{
//	std::cout << "parse_line: \"" << line << "\"" << std::endl;
	line_inc();
	if (line == "{") {
		std::unique_lock<std::mutex> l{m};
		if (!is_empty() && (brace_cnt == 0))
			flush();
		++brace_cnt;
		return;
	}
	else if (line == "}") {
		std::unique_lock<std::mutex> l{m};
		if (brace_cnt > 0) {
			--brace_cnt;
			if (brace_cnt == 0) {
				flush();
				return;
			}
		}
	}
	else
		add(line);

	std::unique_lock<std::mutex> l{m};
	if (is_full() && !brace_cnt)
		flush();
}

void bulk::add(std::string &s)
{
	cmd_inc();
	if (time_first_chunk == 0)
		time_first_chunk = std::time(0);
	vs.push_back(s);
}

void bulk::signal_callback_handler(int signum)
{
	if ((signum == SIGINT) || (signum == SIGTERM))
		flush();
}

bool bulk::is_full(void)
{
	return vs.size() >= bulk_size;
}

bool bulk::is_empty(void)
{
	return vs.size() == 0;
}

#if 0
int main(int argc, char ** argv)
{
	if (argc != 2)
	{
		std::cerr << "Incorrect number of arguments: " << argc - 1 << ", expected: 1" << std::endl;
		return -4;
	}

	size_t j = 0;
	std::string arg = argv[1];
	try {
		std::size_t pos;
		j = std::stoi(arg, &pos);
		if (pos < arg.size()) {
			std::cerr << "Trailing characters after number: " << arg << '\n';
			return -3;
		}
	} catch (std::invalid_argument const &ex) {
		std::cerr << "Invalid number: " << arg << '\n';
		return -1;
	} catch (std::out_of_range const &ex) {
		std::cerr << "Number out of range: " << arg << '\n';
		return -2;
	}

	class bulk b{j};
	printer printerHandler("log");
	saver saverHandler("file1", "file2"/*, "file3", "file4", "file5", "file6", "file7", "file8"*/);

	b.add_handler(printerHandler);
	b.add_handler(saverHandler);
	printerHandler.start_threads();
	saverHandler.start_threads();

	for(std::string line; std::getline(std::cin, line); ) {
		b.parse_line(line);
	}

	printerHandler.stop_threads();
	saverHandler.stop_threads();
	return 0;
}
#endif	// 0

namespace bulki
{
	int descriptor_cnt;
//	std::forward_list<std::pair<int, bulk>> bl;
	std::map<int, std::shared_ptr<bulk>> bm;
	std::shared_timed_mutex smutex;

	int bulka_add(std::size_t bulk_size) {
		std::unique_lock<std::shared_timed_mutex> l{smutex};
		++descriptor_cnt;	// numeration starts from 1
		bool result = false;
//		/*std::tie(std::ignore, result) = */bm.emplace(descriptor_cnt, bulk(bulk_size));
//		/*std::tie(std::ignore, result) = */bm.emplace(descriptor_cnt, bulk_size);
//		bulk b{bulk_size};
		bm.emplace(std::make_pair(descriptor_cnt, std::make_shared<bulk>(bulk_size)));
		return descriptor_cnt;
	}

	void bulka_feed(int descriptor, const char *data, std::size_t size){
//		std::shared_lock<std::shared_timed_mutex> l{smutex};
		std::unique_lock<std::shared_timed_mutex> l{smutex};
		// no exception handling. Should be handled higher
		auto &a = bm.at(descriptor);

		std::stringstream ss(std::string(data, size));
		for(std::string line; std::getline(ss, line); ) {
			a->parse_line(line);
		}
	}


	void bulka_delete(int descriptor){
		std::unique_lock<std::shared_timed_mutex> l{smutex};
		bm.erase(descriptor);
	}
};


namespace async {

handle_t connect(std::size_t bulk_size) {
	int ret = bulki::bulka_add(bulk_size);
	std::cout << "bulka add " << ret << std::endl;
	return ret;
}

void receive(handle_t handle, const char *data, std::size_t size) {
	std::cout << "bulka(" << handle << ") feed: " << std::string(data, size) << std::endl;
	bulki::bulka_feed(handle, data, size);
}

void disconnect(handle_t handle) {
	std::cout << "bulka(" << handle << ") delete" << std::endl;
	bulki::bulka_delete(handle);
}

}



