#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <atomic>

// 컴파일 타임 원형 버퍼 크기 정의 (사용자가 4, 5, 6 등으로 변경 가능)
#ifndef RING_BUFFER_SIZE
#define RING_BUFFER_SIZE 4
#endif

// Thread-safe MFC Windows Message Identifiers
#define WM_USER_SIM_UPDATE (WM_USER + 100)
#define WM_USER_SIM_LOG    (WM_USER + 101)

struct Semaphore {
    int value;
    std::string name;
    std::mutex m;
    std::condition_variable cv;
    std::vector<std::string> waitingThreads;
    HWND hWndNotify; // HWND of the View to notify when state changes

    Semaphore(int value, std::string name) : value(value), name(name), hWndNotify(NULL) {}

    void P(std::string threadName, std::atomic<bool>& terminateFlag) {
        std::unique_lock<std::mutex> lock(m);
        if (value <= 0) {
            waitingThreads.push_back(threadName);
            if (hWndNotify && ::IsWindow(hWndNotify)) {
                ::PostMessage(hWndNotify, WM_USER_SIM_UPDATE, 0, 0);
            }
            
            while (value <= 0 && !terminateFlag) {
                cv.wait(lock);
            }
            
            auto it = std::find(waitingThreads.begin(), waitingThreads.end(), threadName);
            if (it != waitingThreads.end()) {
                waitingThreads.erase(it);
            }
            if (hWndNotify && ::IsWindow(hWndNotify)) {
                ::PostMessage(hWndNotify, WM_USER_SIM_UPDATE, 0, 0);
            }

            if (terminateFlag) {
                return; // Wake up and exit immediately without acquiring
            }
        }
        --value;
        if (hWndNotify && ::IsWindow(hWndNotify)) {
            ::PostMessage(hWndNotify, WM_USER_SIM_UPDATE, 0, 0);
        }
    }

    void V(std::string threadName) {
        std::lock_guard<std::mutex> lock(m);
        ++value;
        cv.notify_one();
        if (hWndNotify && ::IsWindow(hWndNotify)) {
            ::PostMessage(hWndNotify, WM_USER_SIM_UPDATE, 0, 0);
        }
    }
};

struct RingBuffer {
    std::vector<std::string> buffer;
    int in;
    int out;
    Semaphore nrfull;
    Semaphore nrempty;
    Semaphore mutexP;
    Semaphore mutexC;

    RingBuffer() : 
        in(0), out(0),
        nrfull(0, "nrfull"), nrempty(RING_BUFFER_SIZE, "nrempty"),
        mutexP(1, "mutexP"), mutexC(1, "mutexC") 
    {
        buffer.assign(RING_BUFFER_SIZE, "");
    }

    void SetNotificationWindow(HWND hWnd) {
        nrfull.hWndNotify = hWnd;
        nrempty.hWndNotify = hWnd;
        mutexP.hWndNotify = hWnd;
        mutexC.hWndNotify = hWnd;
    }

    void Initialize(HWND hWnd) {
        buffer.assign(RING_BUFFER_SIZE, "");
        in = 0;
        out = 0;

        SetNotificationWindow(hWnd);

        // Reset and re-initialize semaphore values for the compile-time size
        {
            std::lock_guard<std::mutex> lk1(nrfull.m);
            nrfull.value = 0;
            nrfull.waitingThreads.clear();
        }
        {
            std::lock_guard<std::mutex> lk2(nrempty.m);
            nrempty.value = RING_BUFFER_SIZE; // nrempty starts with the total empty slots (RING_BUFFER_SIZE)
            nrempty.waitingThreads.clear();
        }
        {
            std::lock_guard<std::mutex> lk3(mutexP.m);
            mutexP.value = 1;
            mutexP.waitingThreads.clear();
        }
        {
            std::lock_guard<std::mutex> lk4(mutexC.m);
            mutexC.value = 1;
            mutexC.waitingThreads.clear();
        }
    }

    void Reset() {
        Initialize(nrfull.hWndNotify);
    }
};
