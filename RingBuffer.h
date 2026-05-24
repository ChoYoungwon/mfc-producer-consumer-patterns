#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <thread>

#define RING_BUFFER_SIZE 4

template <typename T>
struct RingBuffer {
	T buffer[RING_BUFFER_SIZE];
	int in;
	int out;
	int nrfull;
	int nrempty;
	int mutexP;
	int mutexC;
};

void P(int S) {
	if (S > 0) --S;
	else ;
}

void V() {

}

template <typename T>
class RingBufferManager {
public:
	RingBufferManager(size_t num_threads);
	~RingBufferManager();

	void producer();
	void consumer();

private:
	int count;
	size_t num_threads;
	RingBuffer<T> rb;

	std::vector<std::thread> producer_threads;
	std::vector<std::thread> producer_threads;
};

template <typename T>
RingBufferManager<T>::RingBufferManager(size_t num_threads) : num_threads(num_threads), count(0) {
	rb.nrfull = 0;
	rb.nrempty = RING_BUFFER_SIZE;
	rb.mutexP = 1;
	rb.mutexC = 1;
	rb.in = 0;
	rb.out = 0;
}

template <typename T>
void RingBufferManager<T>::producer() {
	std::string content = std::to_string(count++) + "번 데이터 적재 완료\n";
	P(rb.mutexP);
	P(rb.nrempty);
	rb.buffer[rb.in] = content;
}

template <typename T>
void RingBufferManager<T>::consumer() {

}