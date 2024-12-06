#include <windows.h>
#include <process.h>
#include <iostream>
#include <atomic>
#include <fstream>
#include <iomanip>
#pragma comment(lib, "Winmm.lib")


#include "CircularQueue.h"  // 기존 CircularQueue 헤더 포함 (FIFO 큐)
#include "MemoryPool.h"

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

    bool CAS(UINT64* target, UINT64 expected, UINT64 desired) {
        return InterlockedCompareExchange64(reinterpret_cast<LONG64*>(target), desired, expected) == expected;
    }

public:
    LockFreeStack() : top(0), stamp(0) {}

    ~LockFreeStack() {
        T value;
        while (Pop(value));
    }

    void Push(const T& value)
    {
        Node* newNode = pool.Alloc();
        newNode->data = value;

        Node* currentNode;

        UINT64 stValue = InterlockedIncrement(&stamp);
        UINT64 currentTop;
        UINT64 newTop;

        while (true) {
            currentTop = top;
            currentNode = AddressConverter::ExtractNode(currentTop);

            newNode->next = currentNode; // 새로운 노드의 next를 현재 top으로 설정

            newTop = AddressConverter::AddStamp(newNode, stValue);

            if (CAS(&top, currentTop, newTop)) {
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

        while (true) {
            currentTop = top;
            currentNode = AddressConverter::ExtractNode(currentTop);

            if (!currentNode) {
                return false; // 스택이 비어 있음
            }

            nextNode = currentNode->next;
            newTop = AddressConverter::AddStamp(nextNode, stValue);

            if (CAS(&top, currentTop, newTop)) {
                value = currentNode->data;
                pool.Free(currentNode);
                return true; // 성공적으로 Pop 완료
            }
        }
    }


public:
    UINT32 GetCurPoolCount(void) { return pool.GetCurPoolCount(); }
    UINT32 GetMaxPoolCount(void) { return pool.GetMaxPoolCount(); }

public:
    //CircularQueue<DebugNode> queue;

private:
    UINT64 top; // Top을 나타내는 tagged pointer
    UINT64 stamp; // 기준이 되는 stamp 값

    MemoryPool<Node, false> pool;
};


// 글로벌 스택 인스턴스
LockFreeStack<int> g_Stack;

CRITICAL_SECTION cs;
//
//void handler()
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
//        outFile << std::dec << std::setw(5) << std::setfill('0') << i + 1 << ". Action: ";
//        
//        switch (node.action)
//        {
//        case ACTION::PUSH_BEFORE_CAS:
//            outFile << "PUSH_BEFORE_CAS";
//            break;
//        case ACTION::PUSH_AFTER_CAS:
//            outFile << "PUSH_AFTER_CAS";
//            break;
//        case ACTION::POP_BEFORE_CAS:
//            outFile << "POP_BEFORE_CAS";
//            break;
//        case ACTION::POP_AFTER_CAS:
//            outFile << "POP_AFTER_CAS";
//            break;
//        case ACTION::END:
//            break;
//        default:
//            break;
//        }
//        
//        outFile << ", Thread: " << node.threadId
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

bool bExitWorker = false;
bool bExitMonitor = false;

// 작업자 스레드 함수
unsigned int WINAPI Worker(void* pArg) {
    const int cnt = 10000;// rand() % 1000;

    while (!bExitWorker) {
        for (int i = 0; i < cnt; ++i) {
            g_Stack.Push(i);
        }

        int value;
        for (int i = 0; i < cnt; ++i) {
            if (!g_Stack.Pop(value))
            {
                //break;
                DebugBreak();
                //handler();
                //throw std::exception();
            }
        }
    }

    return 0;
}





// 인자로 받은 서버의 TPS 및 보고 싶은 정보를 1초마다 감시하는 스레드
unsigned int WINAPI MonitorThread(void* pArg)
{
    while (!bExitMonitor)
    {
        std::cout << "===================================\n";

        std::cout << "CurPoolCount : " << g_Stack.GetCurPoolCount() << "\n";
        std::cout << "MaxPoolCount : " << g_Stack.GetMaxPoolCount() << "\n";

        std::cout << "===================================\n\n";

        // 1초간 Sleep
        Sleep(1000);
    }

    return 0;
}







// 메인 함수
int main(void) {

    timeBeginPeriod(1);

    InitializeCriticalSection(&cs);

    const int ThreadCnt = 4;
    HANDLE hHandle[ThreadCnt + 1];

    for (int i = 1; i <= ThreadCnt; ++i) {
        hHandle[i] = (HANDLE)_beginthreadex(NULL, 0, Worker, NULL, 0, NULL);
    }
    // 모니터 스레드 생성
    hHandle[0] = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, NULL, 0, NULL);

    WCHAR ControlKey;

    while (1)
    {
        ControlKey = _getwch();
        if (ControlKey == L'q' || ControlKey == L'Q')
        {
            bExitWorker = true;
            break;
        }
    }

    WaitForMultipleObjects(ThreadCnt, &hHandle[1], TRUE, INFINITE);

    bExitMonitor = true;

    WaitForSingleObject(hHandle[0], INFINITE);


    std::cout << "===================================\n";

    std::cout << "CurPoolCount : " << g_Stack.GetCurPoolCount() << "\n";
    std::cout << "MaxPoolCount : " << g_Stack.GetMaxPoolCount() << "\n";

    std::cout << "===================================\n\n";
    std::cout << "프로세스 종료" << "\n";

    return 0;
}