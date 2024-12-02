#pragma once

#define CQSIZE 1000

#include <Windows.h>

template <typename T>
class CircularQueue {
public:
    // ������
    CircularQueue(UINT32 size = CQSIZE) : capacity(size), count(0){
        //queue = new T[capacity];  // ���� �迭 �Ҵ�
    }

    // �Ҹ���
    ~CircularQueue() {
        //delete[] queue;  // ���� �迭 ����
    }

    // ť�� ������ �߰� (enqueue)
    void enqueue(const T& data) {
        UINT32 inc = InterlockedIncrement(&count);

        UINT32 index = inc % capacity;  // ���� �迭 ó��
        queue[index] = data;
    }

private:
    T queue[CQSIZE];           // ť �迭

    UINT32 count;
    UINT32 capacity;    // ť�� �ִ� ũ��
};
