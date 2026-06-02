#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <atomic>

#define RING_BUFFER_SIZE 4
#define STR_SIZE 100

// Thread-safe MFC Windows Message Identifiers
#define WM_USER_SIM_UPDATE (WM_USER + 100)
#define WM_USER_SIM_LOG    (WM_USER + 101)

struct Semaphore {
    int value;      // S값
    std::string name;
    std::mutex m;
    std::condition_variable cv;
    std::vector<std::string> waitingThreads;
    HWND hWndNotify;

    Semaphore(int value, std::string name) : value(value), name(name), hWndNotify(NULL) {}

    void P(std::string threadName, std::atomic<bool>& terminateFlag) {
        // 1. 상호 배제 적용(value 변수에 대한 경쟁상태 방지, unique_lock 사용해 sleep시 lock 해제)
        std::unique_lock<std::mutex> lock(m);

        // 2. 진입 조건 검사 : 남은 자원이 없는 경우 (S <= 0) 큐에 대기
        if (value <= 0) {
            waitingThreads.push_back(threadName);

            // 대기자 발생 UI 갱신 알림
            if (hWndNotify && ::IsWindow(hWndNotify)) {
                ::PostMessage(hWndNotify, WM_USER_SIM_UPDATE, 0, 0);
            }
            
            // 자원이 제공 되거나 리셋될때 까지 조건 변수로 대기(Sleep)
            while (value <= 0 && !terminateFlag) {
                cv.wait(lock);
            }
            
            // 3. 깨어난(WakeUp) 후의 처리 
            // 대기열 목록에서 현재 스레드 이름 삭제
            auto it = std::find(waitingThreads.begin(), waitingThreads.end(), threadName);
            if (it != waitingThreads.end()) {
                waitingThreads.erase(it);
            }
            // 대기열 빠져나감 UI 갱신 알림
            if (hWndNotify && ::IsWindow(hWndNotify)) {
                ::PostMessage(hWndNotify, WM_USER_SIM_UPDATE, 0, 0);
            }

            // 리셋 버튼 클릭으로 깨어난 경우 (--value) 하지 않고 종료
            if (terminateFlag) {
                return; 
            }
        }
        --value;
        if (hWndNotify && ::IsWindow(hWndNotify)) {
            ::PostMessage(hWndNotify, WM_USER_SIM_UPDATE, 0, 0);
        }
    }

    void V(std::string threadName) {
        // 상호 배제 적용
        std::lock_guard<std::mutex> lock(m);
        
        // 자원 반납
        ++value;

        // 자원이 생겼으므로, P 연산에서 대기 중인 스레드를 깨운다
        cv.notify_one();

        // 자원 수치 증가 및 깨어난 상태를 반영하는 UI 갱신 알림
        if (hWndNotify && ::IsWindow(hWndNotify)) {
            ::PostMessage(hWndNotify, WM_USER_SIM_UPDATE, 0, 0);
        }
    }
};

struct RingBuffer {
    char buffer[RING_BUFFER_SIZE][STR_SIZE];
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
        memset(buffer, 0, sizeof(buffer));
    }

    void SetNotificationWindow(HWND hWnd) {
        nrfull.hWndNotify = hWnd;
        nrempty.hWndNotify = hWnd;
        mutexP.hWndNotify = hWnd;
        mutexC.hWndNotify = hWnd;
    }

    void Reset() {
        in = 0;
        out = 0;
        memset(buffer, 0, sizeof(buffer));
        {
            // waitingThreads 벡터 접근에서의 충돌 방지
            std::lock_guard<std::mutex> lk1(nrfull.m);
            nrfull.value = 0;
            nrfull.waitingThreads.clear();
        }
        {
            std::lock_guard<std::mutex> lk2(nrempty.m);
            nrempty.value = RING_BUFFER_SIZE;
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
};
