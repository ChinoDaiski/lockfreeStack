#include <windows.h>
#include <process.h>
#include <iostream>
#include <atomic>
#include "CircularQueue.h"  // ���� CircularQueue ��� ���� (FIFO ť)
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
    long long pNode;        // ���� ��� ������
    long long pNext;        // ���� ��� ������
    ACTION action;          // ���� ���� (PUSH, POP)
    DWORD threadId;         // ������ ID
    bool casSuccess;        // CAS ���� ����
    unsigned long long timestamp;    // Ÿ�ӽ����� (�и���)

    _tagDebugNode() {}
    _tagDebugNode(long long a, long long b, ACTION _action, DWORD _threadId, bool _success, unsigned long long _timestamp)
        : pNode{ a }, pNext{ b }, action{ _action }, threadId{ _threadId }, casSuccess{ _success }, timestamp{ _timestamp } {}
} DebugNode, * PDebugNode;

// CAS �Լ� ����
template<typename T>
bool CAS(Node<T>** ptr, Node<T>* old_value, Node<T>* new_value) {
    return InterlockedCompareExchange64(
        reinterpret_cast<LONG64*>(ptr),
        reinterpret_cast<LONG64>(new_value),
        reinterpret_cast<LONG64>(old_value)
    ) == reinterpret_cast<LONG64>(old_value);
}

// LockFreeStack Ŭ����
template<typename T>
class LockFreeStack {
public:
    LockFreeStack();
    ~LockFreeStack();

    bool push(Node<T>* pNewNode);
    bool pop(T& value);

private:
    Node<T>* pTop;  // ������ top ���

public:
    CircularQueue<DebugNode> queue;  // ����� ���� ť
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
        // ���� top ��������
        pNode = pTop;
        pNewNode->pNext = pNode;  // ���ο� ����� next�� ���� top���� ����
        casSuccess = CAS(&pTop, pNode, pNewNode);

    } while (!casSuccess);

    // ����� ���� ���
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

        if (pOldTop == nullptr)  // ������ ��� ������ pop ����
        {
            // ����� ���� ���
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

        pNext = pOldTop->pNext;  // ���� ��� ��������
        casSuccess = CAS(&pTop, pOldTop, pNext);

    } while (!casSuccess);

    // ����� ���� ���
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

// �۷ι� ���� �ν��Ͻ�
LockFreeStack<int> g_Stack;
CRITICAL_SECTION cs;

void handler(void)
{
    // ���
    EnterCriticalSection(&cs);

    // ����� ���� ���
    DebugNode node;

    std::ofstream outFile{ "debug_error.txt" };

    if (!outFile)
        std::cerr << "���� ���� ����...\n";

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

// �۾��� ������ �Լ�
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

// ���� �Լ�
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

