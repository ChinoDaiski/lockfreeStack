#include <windows.h>
#include <process.h>
#include <iostream>
#include <atomic>
#include "CircularQueue.h"  // 기존 CircularQueue 헤더 포함 (FIFO 큐)
#include <fstream>
#include <iomanip>
#pragma comment(lib, "Winmm.lib")

void handler(void);

enum class ACTION {
    PUSH,
    POP,
    END
};

template<typename T>
struct Node {
    T data;
    Node<T>* pNext;

    Node(T t) : data{ t }, pNext{ nullptr } {}
};

typedef struct _tagDebugNode {
    long long pNode;        // 이전 노드 포인터
    long long pNext;        // 다음 노드 포인터
    ACTION action;          // 동작 유형 (PUSH, POP)
    DWORD threadId;         // 스레드 ID
    bool casSuccess;        // CAS 성공 여부
    unsigned long long timestamp;    // 타임스탬프 (밀리초)

    _tagDebugNode() {}
    _tagDebugNode(long long a, long long b, ACTION _action, DWORD _threadId, bool _success, unsigned long long _timestamp)
        : pNode{ a }, pNext{ b }, action{ _action }, threadId{ _threadId }, casSuccess{ _success }, timestamp{ _timestamp } {}
} DebugNode, * PDebugNode;

// CAS 함수 정의
template<typename T>
bool CAS(Node<T>** ptr, Node<T>* old_value, Node<T>* new_value) {
    return InterlockedCompareExchange64(
        reinterpret_cast<LONG64*>(ptr),
        reinterpret_cast<LONG64>(new_value),
        reinterpret_cast<LONG64>(old_value)
    ) == reinterpret_cast<LONG64>(old_value);
}

// LockFreeStack 클래스
template<typename T>
class LockFreeStack {
public:
    LockFreeStack();
    ~LockFreeStack();

    bool push(Node<T>* pNewNode);
    bool pop(T& value);

private:
    Node<T>* pTop;  // 스택의 top 노드

public:
    CircularQueue<DebugNode> queue;  // 디버깅 정보 큐
};

template<typename T>
LockFreeStack<T>::LockFreeStack()
    : pTop{ nullptr } {}

template<typename T>
LockFreeStack<T>::~LockFreeStack() {
    while (pTop) {
        Node<T>* temp = pTop;
        pTop = pTop->pNext;
        delete temp;
    }
}

template<typename T>
bool LockFreeStack<T>::push(Node<T>* pNewNode) {
    Node<T>* pNode = nullptr;
    bool casSuccess = false;

    do {
        // 현재 top 가져오기
        pNode = pTop;
        pNewNode->pNext = pNode;  // 새로운 노드의 next를 현재 top으로 설정
        casSuccess = CAS(&pTop, pNode, pNewNode);

    } while (!casSuccess);

    // 디버깅 정보 기록
    queue.enqueue(DebugNode{
        reinterpret_cast<long long>(pNode),
        reinterpret_cast<long long>(pNewNode),
        ACTION::PUSH,
        GetCurrentThreadId(),
        casSuccess,
        GetTickCount64()
        });

    return true;
}

template<typename T>
bool LockFreeStack<T>::pop(T& value) {
    Node<T>* pOldTop = nullptr;
    Node<T>* pNext = nullptr;
    bool casSuccess = false;

    do {
        pOldTop = pTop;

        if (pOldTop == nullptr)  // 스택이 비어 있으면 pop 실패
        {
            // 디버깅 정보 기록
            queue.enqueue(DebugNode{
                reinterpret_cast<long long>(pOldTop),
                reinterpret_cast<long long>(pNext),
                ACTION::POP,
                GetCurrentThreadId(),
                casSuccess,
                GetTickCount64()
                });

            return false;
        }

        pNext = pOldTop->pNext;  // 다음 노드 가져오기
        casSuccess = CAS(&pTop, pOldTop, pNext);

    } while (!casSuccess);

    // 디버깅 정보 기록
    queue.enqueue(DebugNode{
        reinterpret_cast<long long>(pOldTop),
        reinterpret_cast<long long>(pNext),
        ACTION::POP,
        GetCurrentThreadId(),
        casSuccess,
        GetTickCount64()
        });

    value = pOldTop->data;
    delete pOldTop;

    return true;
}

// 글로벌 스택 인스턴스
LockFreeStack<int> g_Stack;
CRITICAL_SECTION cs;

void handler(void)
{
    // 출력
    EnterCriticalSection(&cs);

    // 디버깅 정보 출력
    DebugNode node;

    std::ofstream outFile{ "debug_error.txt" };

    if (!outFile)
        std::cerr << "파일 열기 실패...\n";

    for (int i = 0; i < CQSIZE; ++i)
    {
        node = g_Stack.queue.dequeue();

        outFile << std::dec << std::setw(4) << std::setfill('0') << i + 1 << ". Action: " << (node.action == ACTION::PUSH ? "PUSH" : "POP")
            << ", Thread: " << node.threadId
            << ", CAS Success: " << (node.casSuccess ? "Yes" : "No")
            << ", Timestamp: " << node.timestamp
            << ", Old Node: " << std::hex << std::hex << node.pNode
            << ", New Node: " << std::hex << node.pNext
            << std::endl;
    }

    outFile.close();

    exit(-1); 
}

// 작업자 스레드 함수
unsigned int WINAPI Worker(void* pArg) {
    int cnt = 5;// rand() % 1000;

    while (1) {
        for (int i = 0; i < cnt; ++i) {
            Node<int>* pNode = new Node<int>(i);
            g_Stack.push(pNode);
        }

        int value;
        for (int i = 0; i < cnt; ++i) {
            if (!g_Stack.pop(value))
            {
                handler();
                //throw std::exception();
            }
        }
    }

    return 0;
}

// 메인 함수
int main(void) {

    timeBeginPeriod(1);

    InitializeCriticalSection(&cs);

    const int ThreadCnt = 4;
    HANDLE hHandle[ThreadCnt];

    for (int i = 0; i < ThreadCnt; ++i) {
        hHandle[i] = (HANDLE)_beginthreadex(NULL, 0, Worker, NULL, 0, NULL);
    }

    WaitForMultipleObjects(ThreadCnt, hHandle, TRUE, INFINITE);

    return 0;
}

