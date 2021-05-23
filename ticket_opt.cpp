#include <iostream>
#include <mutex>
#include <thread>
#include <cmath>
#include <atomic>
#include "assert.h"
#include <typeinfo>
#include <immintrin.h> 
#include <vector>
using namespace std;

class Timer {
public:
        Timer(string text) : text_(text) {
                start_ = std::chrono::steady_clock::now();
        }
        ~Timer() {
                auto end = std::chrono::steady_clock::now();
                auto dur = end - start_;
                cerr << text_ << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()
                        << " ms" << endl;
        }
private:
        string text_;
        std::chrono::time_point<std::chrono::steady_clock> start_;
};
class my_mutex{
mutex mmtx;
public:
my_mutex(){}
~my_mutex(){}
string name(){return "Mutex                                  | ";}
void lock(){mmtx.lock();}
void unlock(){mmtx.unlock();}
};

class ticket_lock{
	atomic<unsigned int> now_serving = { 0 };
	atomic<unsigned int>  next_ticket = { 0 };
	public:
	string name(){return "ticket lock                            | ";}
	void lock(){
		const auto ticket = next_ticket.fetch_add(1,memory_order_relaxed);
		while(now_serving.load(memory_order_acquire)!=ticket);
	}	
	void unlock(){
		const auto successor  = now_serving.load(memory_order_relaxed)+1;
		now_serving.store(successor, memory_order_release);
	}
};

class yield_ticket_lock{
	atomic<unsigned int> now_serving = { 0 };
	atomic<unsigned int>  next_ticket = { 0 };
	public:
	string name(){return "ticket lock with yield                 | ";}
	void lock(){
		const auto my_ticket = next_ticket.fetch_add(1,memory_order_relaxed);
		while(now_serving.load(memory_order_acquire)!=my_ticket){
				_mm_pause();			
				this_thread::yield();
					}
	}	
	void unlock(){
		const auto successor  = now_serving.load(memory_order_relaxed)+1;
		now_serving.store(successor, memory_order_release);
	}
};

class prop_bo_ticket_lock{
	atomic<unsigned int> now_serving = { 0 };
	atomic<unsigned int>  next_ticket = { 0 };
	public:
	string name(){return "ticket lock with proportional back off | ";}
	void lock(){
		const auto my_ticket = next_ticket.fetch_add(1,memory_order_relaxed);
		while(now_serving.load(memory_order_acquire)!=my_ticket){
			_mm_pause();		
			const size_t now_between=my_ticket - now_serving;
			int time =0.001*now_between+rand()%1000;
			this_thread::sleep_for(chrono::microseconds(time));
		}
	}	
	void unlock(){
		const auto successor  = now_serving.load(memory_order_relaxed)+1;
		now_serving.store(successor, memory_order_release);
	}
};
ticket_lock ticket;
prop_bo_ticket_lock ticket_prop;
yield_ticket_lock ticket_yield;
my_mutex mtx;
template <typename T>
void Block_Function(T& waiting, const long long& count) {
        waiting.lock();
        for (long long volatile i = 0; i < count; i++) {}
        waiting.unlock();
}

template <typename T>
void Benchmark(T& waiting, size_t counter, const long long& num_iterations) {
        vector<thread> threads;
        {
                	Timer timer(to_string(counter)+" threads | "+ waiting.name() + " ");
               		for (auto i = 0; i < counter; i++) {
                        threads.push_back(thread(Block_Function<T>, ref(waiting), cref(num_iterations)));
                	}

                	for (auto& th : threads)
                        	if (th.joinable())
                                	th.join();
                }
        }
int main(){
	long long count = 1000'000'0;
	size_t num_threads = 100; 
	for(auto i=10; i<= num_threads; i+=10){
	Benchmark<my_mutex>(mtx, i, count);
	Benchmark<ticket_lock>(ticket, i, count);
	Benchmark<prop_bo_ticket_lock>(ticket_prop, i, count);
	Benchmark<yield_ticket_lock>(ticket_yield, i, count);
	}
	return 0;
}

