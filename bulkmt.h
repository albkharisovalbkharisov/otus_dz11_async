#pragma once

#include <thread>
#include <string>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <list>
#include <atomic>
#include <vector>
#include <utility>	// std::move
#include <sstream>

template<bool count_lines_too = false>
class dbg_counter
{
	std::atomic<size_t> line_counter;
	std::atomic<size_t> cmd_counter;
	std::atomic<size_t> bulk_counter;
public:
	dbg_counter(void) : line_counter(0), cmd_counter(0), bulk_counter(0) {}
	void line_inc(size_t i = 1) { line_counter += i; }
	void cmd_inc(size_t i = 1)  { cmd_counter  += i; }
	void bulk_inc(size_t i = 1) { bulk_counter += i; }

	dbg_counter(const dbg_counter &that);
	void print_counters(const std::string &thread_name) const;
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
private:
	std::condition_variable cv;
	// cv_m is mutex to protect qMsg - queue to send data from main thread to worker threads
	std::mutex cv_m;
	std::queue<type_to_handle> qMsg;
	std::list<worker> vThread;
	std::atomic<bool> exit;

	void work(struct worker &w);

public:
	virtual void handle(const type_to_handle &ht) { (void) ht; throw; }

	template<typename ...Names>
	IbaseClass(Names... names) : exit(false) {
		const char * dummy[] = {(const char*)(names)...};
		for (auto &s : dummy)
			vThread.emplace_back(s);
	}

	void start_threads(void);
	void stop_threads(void);
	virtual ~IbaseClass(void);
	void notify(type_to_handle &ht);
};


class bulk : public dbg_counter<true>
{
	const size_t bulk_size;
	std::vector<std::string> vs;
	std::list<std::shared_ptr<IbaseClass>> lHandler;
	std::atomic<size_t> brace_cnt;
	std::time_t time_first_chunk;
	// m is mutex to protect concurent access to one bulk
	std::mutex m;
//	std::stringstream ss;
	std::string ss;

	void flush(void);
	bool is_full(void);
	bool is_empty(void);
	void add(const std::string &s);
	void parse_line(const std::string &line);

public:
	bulk(size_t size);
	void collect_and_parse(const char * data, size_t size);
	~bulk();
};

