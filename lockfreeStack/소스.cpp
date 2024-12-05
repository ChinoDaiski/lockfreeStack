#include <windows.h>
#include <process.h>
#include <iostream>
#include <atomic>
#include "CircularQueue.h"  // 기존 CircularQueue 헤더 포함 (FIFO 큐)
#include <fstream>
#include <iomanip>
#pragma comment(lib, "Winmm.lib")
//
//void handler(void);
//
//enum class ACTION {
//    PUSH,
//    POP,
//    END
//};
//
//template<typename T>
//class alignas(8) Node
//{
//public:
//    Node(T t);
//    ~Node(void);
//
//public:
//    Node<T>* get_next()
//    {
//        return pNext;
//    }
//
//public:
//    const bool operator==(const Node& rhs) {
//        return rhs.pNext == pNext;
//    }
//    const bool operator!=(const Node& rhs) {
//        return !operator==(rhs);
//    }
//
//public:
//    Node<T>* pNext;
//    T data;
//};
//
//template<typename T>
//Node<T>::Node(T t)
//    : data{ t }
//{
//}
//
//template<typename T>
//Node<T>::~Node(void)
//{
//}
//
//
//class AddressConvertor
//{
//public:
//    static UINT64 SetStamp(void* pNode, UINT32 stamp)
//    {
//        // 포인터를 UINT64로 변환 (64비트 정수)
//        UINT64 pointerValue = reinterpret_cast<UINT64>(pNode);
//
//        // 스탬프는 상위 17비트에 저장
//        UINT64 stampValue = static_cast<UINT64>(stamp) << 47;
//
//        // 하위 47비트와 상위 17비트를 결합
//        return pointerValue | stampValue;
//    }
//
//    static void* RemoveStamp(UINT64 pNode)
//    {
//        // 하위 47비트만 유지 (상위 17비트를 제거)
//        UINT64 pointerValue = pNode & ((1ULL << 47) - 1);
//
//        // 순수한 포인터로 변환
//        return reinterpret_cast<void*>(pointerValue);
//    }
//};
//
//typedef struct _tagDebugNode {
//    long long pNode;        // 이전 노드 포인터
//    long long pNext;        // 다음 노드 포인터
//    ACTION action;          // 동작 유형 (PUSH, POP)
//    DWORD threadId;         // 스레드 ID
//    bool casSuccess;        // CAS 성공 여부
//    unsigned long long timestamp;    // 타임스탬프 (밀리초)
//
//    _tagDebugNode() {}
//    _tagDebugNode(long long a, long long b, ACTION _action, DWORD _threadId, bool _success, unsigned long long _timestamp)
//        : pNode{ a }, pNext{ b }, action{ _action }, threadId{ _threadId }, casSuccess{ _success }, timestamp{ _timestamp } {}
//} DebugNode, * PDebugNode;
//
//// CAS 함수 정의
//template<typename T>
//bool CAS(Node<T>** ptr, Node<T>* old_value, Node<T>* new_value) {
//    return InterlockedCompareExchange64(
//        reinterpret_cast<LONG64*>(ptr),
//        reinterpret_cast<LONG64>(new_value),
//        reinterpret_cast<LONG64>(old_value)
//    ) == reinterpret_cast<LONG64>(old_value);
//}
//
//// LockFreeStack 클래스
//template<typename T>
//class LockFreeStack {
//public:
//    LockFreeStack();
//    ~LockFreeStack();
//
//    bool push(T data);
//    bool pop(T& value);
//
//private:
//    Node<T>* pTop;  // 스택의 top 노드
//
//public:
//    CircularQueue<DebugNode> queue;  // 디버깅 정보 큐
//    UINT32 stamp;
//};
//
//template<typename T>
//LockFreeStack<T>::LockFreeStack()
//    : pTop{ nullptr }, stamp{ 0 } {}
//
//template<typename T>
//LockFreeStack<T>::~LockFreeStack() {
//    while (pTop) {
//        Node<T>* temp = pTop;
//        pTop = pTop->pNext;
//        delete temp;
//    }
//}
//
//template<typename T>
//bool LockFreeStack<T>::push(T data) {
//    Node<T>* pNode = NULL;
//    Node<T>* pNewNode = new Node<T>(data);
//
//    bool casSuccess = false;
//    UINT32 retval = InterlockedIncrement(&stamp);
//    pNode = reinterpret_cast<Node<T>*>(AddressConvertor::SetStamp(pNode, retval));
//
//    do {
//        // 현재 top 가져오기
//        pNode = pTop;
//        pNewNode->pNext = pNode;  // 새로운 노드의 next를 현재 top으로 설정
//        casSuccess = CAS(&pTop, pNode, pNewNode);
//
//    } while (!casSuccess);
//
//    // 디버깅 정보 기록
//    queue.enqueue(DebugNode{
//        reinterpret_cast<long long>(pNode),
//        reinterpret_cast<long long>(pNewNode),
//        ACTION::PUSH,
//        GetCurrentThreadId(),
//        casSuccess,
//        GetTickCount64()
//        });
//
//    return true;
//}
//
//template<typename T>
//bool LockFreeStack<T>::pop(T& value) {
//    Node<T>* pOldTop = nullptr;
//    Node<T>* pNext = nullptr;
//    bool casSuccess = false;
//
//    do {
//        pOldTop = pTop;
//
//        if (pOldTop == nullptr)  // 스택이 비어 있으면 pop 실패
//        {
//            // 디버깅 정보 기록
//            queue.enqueue(DebugNode{
//                reinterpret_cast<long long>(pOldTop),
//                reinterpret_cast<long long>(pNext),
//                ACTION::POP,
//                GetCurrentThreadId(),
//                casSuccess,
//                timeGetTime()
//                });
//
//            return false;
//        }
//
//        pNext = pOldTop->pNext;  // 다음 노드 가져오기
//        casSuccess = CAS(&pTop, pOldTop, pNext);
//
//    } while (!casSuccess);
//
//    // 디버깅 정보 기록
//    queue.enqueue(DebugNode{
//        reinterpret_cast<long long>(pOldTop),
//        reinterpret_cast<long long>(pNext),
//        ACTION::POP,
//        GetCurrentThreadId(),
//        casSuccess,
//        timeGetTime()
//        });
//
//    pOldTop = reinterpret_cast<Node<T>*>(AddressConvertor::RemoveStamp(reinterpret_cast<UINT64>(pOldTop)));
//    value = pOldTop->data;
//    delete pOldTop;
//
//    return true;
//}
//
// 
//
//void handler(void)
//{
//    // 출력
//    EnterCriticalSection(&cs);
//
//    // 디버깅 정보 출력
//    DebugNode node;
//
//    std::ofstream outFile{ "debug_error.txt" };
//
//    if (!outFile)
//        std::cerr << "파일 열기 실패...\n";
//
//    for (int i = 0; i < CQSIZE; ++i)
//    {
//        node = g_Stack.queue.dequeue();
//
//        outFile << std::dec << std::setw(4) << std::setfill('0') << i + 1 << ". Action: " << (node.action == ACTION::PUSH ? "PUSH" : "POP")
//            << ", Thread: " << node.threadId
//            << ", CAS Success: " << (node.casSuccess ? "Yes" : "No")
//            << ", Timestamp: " << node.timestamp
//            << ", Old Node: " << std::hex << std::hex << node.pNode
//            << ", New Node: " << std::hex << node.pNext
//            << std::endl;
//    }
//
//    outFile.close();
//
//    exit(-1); 
//}
//




#include <iostream>
#include <cstdint>
#include <Windows.h>


void handler();

int filter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
{
    puts("in filter.");
    if (code == EXCEPTION_ACCESS_VIOLATION)
    {
        puts("caught AV as expected.");
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        puts("didn't catch AV, unexpected.");
        return EXCEPTION_CONTINUE_SEARCH;
    };
}



enum class ACTION {
    PUSH_BEFORE_CAS,
    PUSH_AFTER_CAS,
    POP_BEFORE_CAS,
    POP_AFTER_CAS,
    END
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





template <typename T>
class LockFreeStack {
private:
    struct Node {
        T data;
        Node* next;

        Node(T _data, Node* _next) : data{ _data }, next{ _next } {}
    };

    struct AddressConverter {
        static constexpr UINT64 POINTER_MASK = 0x00007FFFFFFFFFFF; // 하위 47비트
        static constexpr UINT64 STAMP_SHIFT = 47;

        // 노드 주소에 stamp 추가
        static UINT64 AddStamp(Node* node, UINT64 stamp) {
            UINT64 pointerValue = reinterpret_cast<UINT64>(node);
            return (pointerValue & POINTER_MASK) | (stamp << STAMP_SHIFT);
        }

        // 노드 주소 추출
        static Node* ExtractNode(UINT64 taggedPointer) {
            return reinterpret_cast<Node*>(taggedPointer & POINTER_MASK);
        }
    };

    UINT64 top; // Top을 나타내는 tagged pointer
    UINT64 stamp; // 기준이 되는 stamp 값

    // Compare-and-Swap for 64-bit tagged pointers
//#define CAS(target, expected, desired) (InterlockedCompareExchange64(reinterpret_cast<LONG64*>(target), desired, expected) == expected);
    bool CAS(UINT64* target, UINT64 expected, UINT64 desired) {
        return InterlockedCompareExchange64(reinterpret_cast<LONG64*>(target), desired, expected) == expected;
    }

public:
    CircularQueue<DebugNode> queue;



public:
    LockFreeStack() : top(0), stamp(0) {}

    ~LockFreeStack() {
        T value;
        while (Pop(value));
    }

    void Push(const T& value)
    {
        Node* newNode = new Node{ value, nullptr }; 
        Node* currentNode;

        UINT64 stValue = InterlockedIncrement(&stamp);
        UINT64 currentTop;
        UINT64 newTop;

        bool casSuccess = false;

        DWORD id = GetCurrentThreadId();

        while (true) {
            currentTop = top;
            currentNode = AddressConverter::ExtractNode(currentTop);

            newNode->next = currentNode; // 새로운 노드의 next를 현재 top으로 설정

            newTop = AddressConverter::AddStamp(newNode, stValue);
            
            // 디버깅 정보 기록
            queue.enqueue(DebugNode{
                reinterpret_cast<long long>(currentNode),
                reinterpret_cast<long long>(newNode),
                ACTION::PUSH_BEFORE_CAS,
                id,
                casSuccess,
                GetTickCount64()
                });


            casSuccess = CAS(&top, currentTop, newTop);

            // 디버깅 정보 기록
            queue.enqueue(DebugNode{
                reinterpret_cast<long long>(currentNode),
                reinterpret_cast<long long>(newNode),
                ACTION::PUSH_AFTER_CAS,
                id,
                casSuccess,
                GetTickCount64()
                });

            if (casSuccess) {
                break; // 성공적으로 Push 완료
            }
        }
    }

    bool Pop(T& value) {
        Node* currentNode; 
        Node* nextNode = nullptr;
        UINT64 currentTop; 
        UINT64 newTop;
        UINT64 stValue = InterlockedIncrement(&stamp);


        bool casSuccess;
        DWORD id = GetCurrentThreadId();

        __try
        {
            while (true) {
                currentTop = top;
                currentNode = AddressConverter::ExtractNode(currentTop);

                // 디버깅 정보 기록
                queue.enqueue(DebugNode{
                reinterpret_cast<long long>(currentNode),
                reinterpret_cast<long long>(nextNode),
                ACTION::POP_BEFORE_CAS,
                id,
                casSuccess,
                timeGetTime()
                    });

                if (!currentNode) {
                    return false; // 스택이 비어 있음
                }

                nextNode = currentNode->next;
                newTop = AddressConverter::AddStamp(nextNode, stValue);

                casSuccess = CAS(&top, currentTop, newTop);

                // 디버깅 정보 기록
                queue.enqueue(DebugNode{
                reinterpret_cast<long long>(currentNode),
                reinterpret_cast<long long>(nextNode),
                ACTION::POP_AFTER_CAS,
                id,
                casSuccess,
                timeGetTime()
                    });

                if (casSuccess) {
                    value = currentNode->data;

                    delete currentNode;
                    return true; // 성공적으로 Pop 완료
                }
            }
        }
        __except(filter(GetExceptionCode(), GetExceptionInformation()))
        {
            handler();
        }

    }
};


// 글로벌 스택 인스턴스
LockFreeStack<int> g_Stack;

CRITICAL_SECTION cs;

void handler()
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

        outFile << std::dec << std::setw(5) << std::setfill('0') << i + 1 << ". Action: ";
        
        switch (node.action)
        {
        case ACTION::PUSH_BEFORE_CAS:
            outFile << "PUSH_BEFORE_CAS";
            break;
        case ACTION::PUSH_AFTER_CAS:
            outFile << "PUSH_AFTER_CAS";
            break;
        case ACTION::POP_BEFORE_CAS:
            outFile << "POP_BEFORE_CAS";
            break;
        case ACTION::POP_AFTER_CAS:
            outFile << "POP_AFTER_CAS";
            break;
        case ACTION::END:
            break;
        default:
            break;
        }
        
        outFile << ", Thread: " << node.threadId
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
    int cnt = 5000;// rand() % 1000;

    while (1) {
        for (int i = 0; i < cnt; ++i) {
            g_Stack.Push(i);
        }

        int value;
        for (int i = 0; i < cnt; ++i) {
            if (!g_Stack.Pop(value))
            {
                break;
                //DebugBreak();
                //handler();
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

    const int ThreadCnt = 2;
    HANDLE hHandle[ThreadCnt];

    for (int i = 0; i < ThreadCnt; ++i) {
        hHandle[i] = (HANDLE)_beginthreadex(NULL, 0, Worker, NULL, 0, NULL);
    }

    WaitForMultipleObjects(ThreadCnt, hHandle, TRUE, INFINITE);

    return 0;
}
