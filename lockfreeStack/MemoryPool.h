#pragma once

#include <iostream>
#include <chrono>
#include <vector>
#include <new>
#include <Windows.h>

//#include "Profile.h"

#include "CircularQueue.h"

#define GUARD_VALUE 0xAAAABBBBCCCCDDDD

// Node ����ü ����

#ifdef _DEBUG
#pragma pack(1)
#endif // _DEBUG


template<typename T>
struct Node
{
    // x64 ȯ�濡�� ����ɰ��� ����ؼ� UINT64 ���� ���
#ifdef _DEBUG

    UINT64 BUFFER_GUARD_FRONT;
    T data;
    UINT64 BUFFER_GUARD_END;
    ULONG_PTR POOL_INSTANCE_VALUE;
    Node<T>* next;

#endif // _DEBUG
#ifndef _DEBUG

    T data;
    Node<T>* next;

#endif // !_DEBUG
};

#ifdef _DEBUG
#pragma pop
#endif // _DEBUG

template<typename T>
struct AddressConverter {
    static constexpr UINT64 POINTER_MASK = 0x00007FFFFFFFFFFF; // ���� 47��Ʈ
    static constexpr UINT64 STAMP_SHIFT = 47;

    // ��� �ּҿ� stamp �߰�
    static UINT64 AddStamp(Node<T>* node, UINT64 stamp) {
        UINT64 pointerValue = reinterpret_cast<UINT64>(node);
        return (pointerValue & POINTER_MASK) | (stamp << STAMP_SHIFT);
    }

    // ��� �ּ� ����
    static Node<T>* ExtractNode(UINT64 taggedPointer) {
        return reinterpret_cast<Node<T>*>(taggedPointer & POINTER_MASK);
    }
};

bool CAS(UINT64* target, UINT64 expected, UINT64 desired) {
    return InterlockedCompareExchange64(reinterpret_cast<LONG64*>(target), desired, expected) == expected;
}


//// ����� ���� ���
//queue.enqueue(DebugNode{
//    reinterpret_cast<long long>(pNode),
//    reinterpret_cast<long long>(pNewNode),
//    ACTION::PUSH,
//    GetCurrentThreadId(),
//    casSuccess,
//    GetTickCount64()
//    });

//enum class ACTION {
//    ALLOC,
//    FREE,
//    END
//};

//typedef struct _tagDebugNode {
//    long long pNode;        // ���� ��� ������
//    long long pNext;        // ���� ��� ������
//    ACTION action;          // ���� ���� (PUSH, POP)
//    DWORD threadId;         // ������ ID
//    bool casSuccess;        // CAS ���� ����
//    unsigned long long timestamp;    // Ÿ�ӽ����� (�и���)
//
//    _tagDebugNode() {}
//    _tagDebugNode(long long a, long long b, ACTION _action, DWORD _threadId, bool _success, unsigned long long _timestamp)
//        : pNode{ a }, pNext{ b }, action{ _action }, threadId{ _threadId }, casSuccess{ _success }, timestamp{ _timestamp } {}
//} DebugNode, * PDebugNode;


// MemoryPool Ŭ���� ����
template<typename T, bool bPlacementNew>
class MemoryPool
{
public:
    // ������
    MemoryPool(UINT32 sizeInitialize = 0);

    // �Ҹ���
    virtual ~MemoryPool(void);
    
    // Ǯ�� �ִ� ��ü�� �Ѱ��ְų� ���� �Ҵ��� �ѱ�
    T* Alloc(void);
    
    // ��ü�� Ǯ�� ��ȯ
    bool Free(T* ptr);
   
public:
    UINT32 GetCurPoolCount(void){ return InterlockedCompareExchange(&m_curPoolCount, 0, 0); }
    UINT32 GetMaxPoolCount(void){ return InterlockedCompareExchange(&m_maxPoolCount, 0, 0); }

private:
    //Node<T>* m_freeNode;
    UINT32 m_curPoolCount; // Ǯ���� ����ϴ� ��� ����, Alloc�Ǹ� 1 ����, Free�Ǹ� 1 ����
    UINT32 m_maxPoolCount; // Ǯ���� ����ϴ� �ִ� ��� ����

private:
    UINT64 top; // Top�� ��Ÿ���� tagged pointer, Node<T>* m_freeNode�� �ٲ� ����
    UINT64 stamp; // ������ �Ǵ� stamp ��

//public:
//    CircularQueue<DebugNode> debugQueue;
};

template<typename T, bool bPlacementNew>
inline MemoryPool<T, bPlacementNew>::MemoryPool(UINT32 sizeInitialize)
{
    top = 0;
    m_curPoolCount = 0;
    m_maxPoolCount = 0;
    stamp = 0;

    T** pArr = new T * [sizeInitialize];

    // �ʱ� �޸� ���� �غ�
    if (sizeInitialize > 0)
    {
        for (UINT32 i = 0; i < sizeInitialize; i++)
        {
            pArr[i] = Alloc();
        }
        
        for (UINT32 i = 0; i < sizeInitialize; i++)
        {
            Free(pArr[i]);
        }
    }

    delete[] pArr;
}

template<typename T, bool bPlacementNew>
inline MemoryPool<T, bPlacementNew>::~MemoryPool(void)
{
    Node<T>* currentNode;
    Node<T>* nextNode = nullptr;
    UINT64 currentTop;
    UINT64 newTop;
    UINT64 stValue = InterlockedIncrement(&stamp);

    while (true) {
        currentTop = top;
        currentNode = AddressConverter<T>::ExtractNode(currentTop);

        if (!currentNode) {
            // ������ ��� ����
            break;
        }

        nextNode = currentNode->next;
        newTop = AddressConverter<T>::AddStamp(nextNode, stValue);

        if (CAS(&top, currentTop, newTop)) {
            delete currentNode;
            InterlockedDecrement(&m_maxPoolCount);
        }
    }

    // ���� �Ҵ� ������ ���� �Ϸ���� �ʾҴٸ�
    if (m_maxPoolCount != 0)
    {
        // ��... ������� ���߿� �����ڱ�.
    }
}

// ���� ����ִٸ� �Ҵ�, �ִٸ� pop�ϰ� ��ȯ�ε�...
template<typename T, bool bPlacementNew>
inline T* MemoryPool<T, bPlacementNew>::Alloc(void)
{
    Node<T>* currentNode;
    Node<T>* nextNode = nullptr;
    UINT64 currentTop;
    UINT64 newTop;
    UINT64 stValue = InterlockedIncrement(&stamp);

    while (true) {
        currentTop = top;
        currentNode = AddressConverter<T>::ExtractNode(currentTop);

        // ������ ��� �ִٸ� ���� ��带 �����ؼ� ��ȯ
        if (!currentNode) {
            // m_freeNode�� nullptr�̶�� Ǯ�� ��ü�� �������� �ʴ´ٴ� �ǹ��̹Ƿ� ���ο� ��ü �Ҵ�

    // �� ��� �Ҵ�
            Node<T>* newNode = (Node<T>*)malloc(sizeof(Node<T>));
            ZeroMemory(newNode, sizeof(T));

#ifdef _DEBUG
            // ������ ����. ���� ���� Ȯ���ϰ�, ��ȯ�Ǵ� Ǯ�� ������ �ùٸ��� Ȯ���ϱ� ���� ���
            newNode->BUFFER_GUARD_FRONT = GUARD_VALUE;
            newNode->BUFFER_GUARD_END = GUARD_VALUE;

            newNode->POOL_INSTANCE_VALUE = reinterpret_cast<ULONG_PTR>(this);
#endif // _DEBUG

            newNode->next = nullptr;    // ��Ȯ�� m_freeNode�� �����ص� �ȴ�. �ٵ� �� ��ü�� nullptr�̴� ���� ���� nullptr ����.

            // placement New �ɼ��� �����ִٸ� ������ ȣ��
            if constexpr (bPlacementNew)
            {
                //new (reinterpret_cast<char*>(newNode) + offsetof(Node<T>, data)) T();
                new (&(newNode->data)) T();
            }

            // Ǯ���� ����ϴ� �ִ� ��� ������ 1 ����
            InterlockedIncrement(&m_maxPoolCount);

            // ��ü�� TŸ�� ������ ��ȯ
            return reinterpret_cast<T*>(reinterpret_cast<char*>(newNode) + offsetof(Node<T>, data));
        }

        nextNode = currentNode->next;
        newTop = AddressConverter<T>::AddStamp(nextNode, stValue);

        if (CAS(&top, currentTop, newTop)) {

            // placement New �ɼ��� �����ִٸ� ������ ȣ��
            if constexpr (bPlacementNew)
            {
                //new (reinterpret_cast<char*>(returnNode) + offsetof(Node<T>, data)) T();
                new (&(currentNode->data)) T();
            }

            // Ǯ�� �����ϴ� ��� ������ 1 ����
            InterlockedDecrement(&m_curPoolCount);

            // ��ü�� TŸ�� ������ ��ȯ
            return &currentNode->data;
        }
    }
}

template<typename T, bool bPlacementNew>
inline bool MemoryPool<T, bPlacementNew>::Free(T* ptr)
{
#ifdef _DEBUG
    // ��ȯ�ϴ� ���� �������� �ʴ´ٸ�
    if (ptr == nullptr)
    {
        // ����
        return false;
    }
#endif // _DEBUG

    // offsetof(Node<T>, data)) => Node ����ü�� data ������ Node ����ü���� �ּ� ���̰��� ���
    // ���⼱ debug ����϶� ���尡 �����Ƿ� 4, release�� ��� 0���� ó��
    Node<T>* pNode = reinterpret_cast<Node<T>*>(reinterpret_cast<char*>(ptr) - offsetof(Node<T>, data));

#ifdef _DEBUG 
    // ���� ����, ��� �÷ο� ����
    if (
        pNode->BUFFER_GUARD_FRONT != GUARD_VALUE ||
        pNode->BUFFER_GUARD_END != GUARD_VALUE
        )
    {
        // ���� �ٸ��ٸ� false ��ȯ
        return false;
    }

    //  Ǯ ��ȯ�� �ùٸ��� �˻�
    if (pNode->POOL_INSTANCE_VALUE != reinterpret_cast<ULONG_PTR>(this))
    {
        // �� �������� ��� ����... ���߿� ����.


        // ���� �ٸ��ٸ� false ��ȯ
        return false;
    }
#endif // _DEBUG

    // ���� placement new �ɼ��� ���� �ִٸ�, ������ placement new�� ���� ���̹Ƿ� �ش� ��ü�� �Ҹ��ڸ� �������� �ҷ���
    if constexpr (bPlacementNew)
    {
        pNode->data.~T();
    }

    Node<T>* currentNode;

    UINT64 stValue = InterlockedIncrement(&stamp);
    UINT64 currentTop;
    UINT64 newTop;

    while (true) {
        currentTop = top;
        currentNode = AddressConverter<T>::ExtractNode(currentTop);

        pNode->next = currentNode; // ���ο� ����� next�� ���� top���� ����

        newTop = AddressConverter<T>::AddStamp(pNode, stValue);

        if (CAS(&top, currentTop, newTop)) {
            break; // ���������� Push �Ϸ�
        }
    }

    // Ǯ�� �����ϴ� ��� ������ 1 ����
    InterlockedIncrement(&m_curPoolCount);

    // ��ȯ ����
    return true;
}