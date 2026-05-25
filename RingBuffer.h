#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <cstdio>
#include <condition_variable>
#include <atomic>

#define RING_BUFFER_SIZE 4

std::atomic<int> count{ 1 };

struct Semaphore {
	int value;
	std::mutex m;
	std::condition_variable cv;

	Semaphore(int value) : value(value) {}

	void P(std::string name) {
		std::unique_lock<std::mutex> lock(m);
		while (value <= 0) {
			std::cout << std::endl;
			std::cout << name << "sleep\n" << std::endl;
			cv.wait(lock);
		}
		--value;
	}

	void V(std::string name) {
		std::unique_lock<std::mutex> lock(m);
		++value;
		cv.notify_one();
		std::cout << std::endl;
		std::cout << name << "wakeup\n" << std::endl;
	}
};

template <typename T>
struct RingBuffer {
	T buffer[RING_BUFFER_SIZE];
	int in;
	int out;
	Semaphore nrfull;
	Semaphore nrempty;
	Semaphore mutexP;
	Semaphore mutexC;

	RingBuffer() : 
		in(0), out(0), 
		nrfull(0), nrempty(RING_BUFFER_SIZE),
		mutexP(1), mutexC(1) {}
};

template <typename T>
class RingBufferManager {
public:
	RingBufferManager() : rb() {};
	~RingBufferManager() {};

	void producer();
	void consumer(); 
	void printStatus(const std::string& action);

private:
	RingBuffer<T> rb;
};

template <typename T>
void RingBufferManager<T>::producer() {
	std::string name = std::to_string(count) + "번 생산자";
	std::string content = std::to_string(count) + "번 데이터\n";
	count++;

	rb.nrempty.P(name);
	rb.mutexP.P(name);
	
	rb.buffer[rb.in] = content;
	rb.in = (rb.in + 1) % RING_BUFFER_SIZE;

	printStatus(name + " 데이터 적재 완료");

	rb.mutexP.V(name);
	rb.nrfull.V(name);
}

template <typename T>
void RingBufferManager<T>::consumer() {
	std::string name = std::to_string(count) + "번 소비자";
	count++;
	rb.mutexC.P(name);
	rb.nrfull.P(name);

	std::string content = rb.buffer[rb.out];
	rb.buffer[rb.out] = "";
	rb.out = (rb.out + 1) % RING_BUFFER_SIZE;

	printStatus(name + " 데이터 확인 : " + content);

	rb.nrempty.V(name);
	rb.mutexC.V(name);
}

template <typename T>
void RingBufferManager<T>::printStatus(const std::string& action) {
	// 여러 스레드가 동시에 출력하여 화면이 깨지는 것을 방지
	static std::mutex displayMtx;
	std::lock_guard<std::mutex> lock(displayMtx);

	std::cout << "\n==================================================" << std::endl;
	std::cout << " [현재 동작]: " << action << std::endl;
	std::cout << "--------------------------------------------------" << std::endl;

	// 1. 버퍼 상태 시각화 (A[0] ~ A[3])
	std::cout << " [Buffer State]" << std::endl;
	for (int i = 0; i < RING_BUFFER_SIZE; ++i) {
		std::cout << "  A[" << i << "]: ";
		if (rb.buffer[i].empty()) {
			std::cout << "[  EMPTY  ]";
		}
		else {
			// 개행 문자가 포함되어 있다면 제거하고 출력
			std::string displayContent = rb.buffer[i];
			if (!displayContent.empty() && displayContent.back() == '\n') displayContent.pop_back();
			std::cout << "[ " << displayContent << " ]";
		}

		// 현재 in과 out 포인터 위치 표시
		if (i == rb.in) std::cout << " <-- [in]";
		if (i == rb.out) std::cout << " <-- [out]";
		std::cout << std::endl;
	}

	// 2. 세마포어 및 변수 상태 출력
	std::cout << "--------------------------------------------------" << std::endl;
	std::cout << "  in: " << rb.in << "  |  out: " << rb.out << std::endl;
	// Semaphore의 value는 현재 남은 자원량을 의미함
	std::cout << "  nrfull: " << rb.nrfull.value << " (차 있는 칸)" << std::endl;
	std::cout << "  nrempty: " << rb.nrempty.value << " (비어 있는 칸)" << std::endl;
	std::cout << "  mutexP: " << rb.mutexP.value << " |  mutexC: " << rb.mutexC.value << std::endl;
	std::cout << "==================================================" << std::endl;
}