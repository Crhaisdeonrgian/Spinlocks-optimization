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
string name(){return "Mutex                                | ";}
void lock(){mmtx.lock();}
void unlock(){mmtx.unlock();}
};
class spin_lock_TTAS{
	atomic<unsigned int> m_spin=0;
	public:
	spin_lock_TTAS() {}
	~spin_lock_TTAS() {assert(m_spin.load(memory_order_acquire)==0);}
	string name(){return "Spin lock TTAS time                  | ";}
	void lock(){
		unsigned int expected;
		do{
			while(m_spin.load(memory_order_acquire));
			expected=0;
		}
		while(!m_spin.compare_exchange_weak(expected, 1, memory_order_acq_rel));	}
	void unlock(){
		m_spin.store(0, memory_order_release);
	}

};


class yield_spin_lock_TTAS{
atomic<unsigned int> m_spin=0;
	public:
	yield_spin_lock_TTAS() {}
	~yield_spin_lock_TTAS() {assert(m_spin.load(memory_order_acquire)==0);}
	string name(){return "Spin lock TTAS with yield time       | ";}
	void lock(){
		unsigned int expected=0;
		do{
			while(m_spin.load(memory_order_acquire)){
				_mm_pause();			
			this_thread::yield();
			}
			expected=0;}
		while (!m_spin.compare_exchange_weak(expected, 1, memory_order_acq_rel));
	}
	void unlock(){
		m_spin.store(0, memory_order_release);
	}

};
class exp_bo_spin_lock_TTAS{
atomic<unsigned int> m_spin=0;
	public:
	exp_bo_spin_lock_TTAS(){}
	~exp_bo_spin_lock_TTAS() {assert(m_spin.load(memory_order_acquire)==0);}
	string name(){return "Spin lock TTAS with exp back off     | ";}
	void lock(){
		unsigned int expected=0;
		size_t max_power = 15;
		//double max_time = 0.01*pow(2, max_power);
		double max_time = 6553;
		size_t power =0;
		do{
			while(m_spin.load(memory_order_acquire)){
				_mm_pause();
				int time = min(0.01*pow(2, power) + rand() % 50,max_time+rand()%50);
					power++;
					this_thread::sleep_for(chrono::microseconds(time));
			}
			expected=0;}
		while (!m_spin.compare_exchange_weak(expected, 1));
	}
	void unlock(){
		m_spin.store(0, memory_order_release);
	}

};

class yield_exp_bo_spin_lock_TTAS{
atomic<unsigned int> m_spin=0;
	public:
	yield_exp_bo_spin_lock_TTAS() {}
	~yield_exp_bo_spin_lock_TTAS() {assert(m_spin.load(memory_order_acquire)==0);}
	string name(){return "Spin lock TTAS with yield and exp bo | ";}
	void lock(){
		unsigned int expected=0;
		size_t max_power = 15;
		//double max_time = 0.01*pow(2, max_power);
		atomic<unsigned int> power =0;
		do{
			while(m_spin.load(memory_order_acquire)){
				_mm_pause();
					if(power==0){
						this_thread::yield();
					}
					else{
						int time = min(0.01*pow(2, double(power)) + rand() % 50,double(6553+rand()%50));
                	                	power.store(power.load()+1);
						this_thread::sleep_for(chrono::microseconds(time));
					}
			}
		  expected=0;}
		while (!m_spin.compare_exchange_weak(expected, 1, memory_order_acq_rel))	;	
}
	void unlock(){
		m_spin.store(0, memory_order_release);
	}
};
spin_lock_TTAS TTAS;
exp_bo_spin_lock_TTAS TTAS_exp;
yield_spin_lock_TTAS TTAS_yield;
yield_exp_bo_spin_lock_TTAS TTAS_yield_exp;
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
	Benchmark<spin_lock_TTAS>(TTAS, i, count);
	Benchmark<exp_bo_spin_lock_TTAS>(TTAS_exp, i, count);
	Benchmark<yield_spin_lock_TTAS>(TTAS_yield, i, count);
	Benchmark<yield_exp_bo_spin_lock_TTAS>(TTAS_yield_exp, i, count);
	}
	return 0;
}
