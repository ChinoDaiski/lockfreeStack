#pragma once

#define CQSIZE 2000000

#include <Windows.h>

template <typename T>
class CircularQueue {
public:
    // ������
    CircularQueue(UINT32 size = CQSIZE) : capacity(size), count(0) {
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

    // ������ ����� �� ���ϰ� �Ϸ��� ����ϴ� �Լ�. ���ÿ� ������� ����. ������ ����� ��Ȳ���� ����ϱ� ���϶� ���.
    const T& dequeue(void)
    {
        if (!bDequeue)
        {
            bDequeue = true;
            deqeueCnt = count;
        }

        deqeueCnt++;
        deqeueCnt %= capacity;

        return queue[(deqeueCnt + 1) % capacity];
    }

    UINT32 GetCount(void) { return count; }

private:
    T queue[CQSIZE];           // ť �迭

    UINT32 count;
    UINT32 capacity;    // ť�� �ִ� ũ��

    bool bDequeue = false;
    int deqeueCnt = -1;
};