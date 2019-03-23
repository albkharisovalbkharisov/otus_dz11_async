#include <chrono>
#include <iostream>
#include <fstream>
#include "bulkmt.h"


template<bool count_lines_too>
dbg_counter<count_lines_too>::dbg_counter(const dbg_counter<count_lines_too> &that)
{
	this->line_counter.store(that.line_counter);
	this->cmd_counter.store(that.cmd_counter);
	this->bulk_counter.store(that.bulk_counter);
}

template<bool count_lines_too>
void dbg_counter<count_lines_too>::print_counters(const std::string &thread_name) const
{
	std::cout << "thread " << thread_name << ": ";
	if (count_lines_too)
		std::cout << line_counter << " lines, ";
	std::cout << cmd_counter << " commands, ";
	std::cout << bulk_counter << " bulks" << std::endl;
}

void IbaseClass::work(struct worker &w)
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

void IbaseClass::start_threads(void)
{
	for (auto &w : vThread)
		w.thread = std::thread(&IbaseClass::work, this, std::ref(w));   // moving thread is OK
}

void IbaseClass::stop_threads(void)
{
	exit = true;
	cv.notify_all();
	for (auto &a : vThread) {
		if (a.thread.joinable())
			a.thread.join();
		else {
			std::cout << "stop_threads() : join..." << a.name << " can't!" << std::endl;
			throw "ERROR: thread can't be joined";
		}
	}
}

IbaseClass::~IbaseClass(void)
{
	for (const auto &a : vThread)
		a.dbg.print_counters(a.name);
}

void IbaseClass::notify(type_to_handle &ht)
{
	{
		std::unique_lock<std::mutex> lk(cv_m);
		qMsg.push(ht);
	}
	cv.notify_one();
}

class saver : public IbaseClass
{
public:
	template<typename ...Names>
	saver(Names... names) : IbaseClass(names...) {}
	void handle(const type_to_handle &ht) override {
		using namespace std::chrono;

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

bulk::~bulk(){
	print_counters("main");
	for (const auto &h : lHandler) {
		h->stop_threads();
	}
}

void bulk::flush(void) {
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

bulk::bulk(size_t size) : bulk_size(size), brace_cnt(0), time_first_chunk(0) {
	vs.reserve(bulk_size);
	auto save_ptr = std::make_shared<saver> ("file1", "file2");
	lHandler.push_back(save_ptr);
	save_ptr->start_threads();
	auto print_ptr = std::make_shared<printer> ("log");
	lHandler.push_back(print_ptr);
	print_ptr->start_threads();
}

void bulk::parse_line(const char * data, size_t size)
{
	std::cout << "parse_line: ss=\"" << ss.str() << "\", str=\"" << std::string(data,size) << "\"" << std::endl;
	ss << std::string(data, size);

	using std::string;

	for (string line; (ss.str().find('\n') != std::string::npos) && getline(ss, line); ) {
		std::cout << "		LETS PARSE: \"" << line << "\"" << std::endl;
		line_inc();
		std::unique_lock<std::mutex> l{m};
		if (line == "{") {
			if (!is_empty() && (brace_cnt == 0))
				flush();
			++brace_cnt;
			return;
		}
		else if (line == "}") {
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

		if (is_full() && !brace_cnt)
			flush();
	}
}

void bulk::add(std::string &s)
{
	// protected by m mutex
	cmd_inc();
	if (time_first_chunk == 0)
		time_first_chunk = std::time(0);
	vs.push_back(s);
}

bool bulk::is_full(void)
{
	return vs.size() >= bulk_size;
}

bool bulk::is_empty(void)
{
	return vs.size() == 0;
}

