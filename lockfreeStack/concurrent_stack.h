#pragma once

#include "MemoryPool.h"

template <typename T>
class LockFreeStack {
private:
    struct Node {
        T data;
        Node* next;

        Node(T _data, Node* _next) : data{ _data }, next{ _next } {}
    };

    struct AddressConverter {
        static constexpr UINT64 POINTER_MASK = 0x00007FFFFFFFFFFF; // ���� 47��Ʈ
        static constexpr UINT64 STAMP_SHIFT = 47;

        // ��� �ּҿ� stamp �߰�
        static UINT64 AddStamp(Node* node, UINT64 stamp) {
            UINT64 pointerValue = reinterpret_cast<UINT64>(node);
            return (pointerValue & POINTER_MASK) | (stamp << STAMP_SHIFT);
        }

        // ��� �ּ� ����
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

            newNode->next = currentNode; // ���ο� ����� next�� ���� top���� ����

            newTop = AddressConverter::AddStamp(newNode, stValue);

            if (CAS(&top, currentTop, newTop)) {
                break; // ���������� Push �Ϸ�
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
                return false; // ������ ��� ����
            }

            nextNode = currentNode->next;
            newTop = AddressConverter::AddStamp(nextNode, stValue);

            if (CAS(&top, currentTop, newTop)) {
                value = currentNode->data;
                pool.Free(currentNode);
                return true; // ���������� Pop �Ϸ�
            }
        }
    }


public:
    UINT32 GetCurPoolCount(void) { return pool.GetCurPoolCount(); }
    UINT32 GetMaxPoolCount(void) { return pool.GetMaxPoolCount(); }

private:
    UINT64 top; // Top�� ��Ÿ���� tagged pointer
    UINT64 stamp; // ������ �Ǵ� stamp ��

    MemoryPool<Node, false> pool;
};
