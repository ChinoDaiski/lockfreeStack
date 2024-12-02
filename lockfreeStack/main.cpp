
#include <iostream>
#include <Windows.h>
#include <process.h>

#include "CircularQueue.h"

enum class ACTION {
	PUSH,
	POP, 
	END
};

template<typename T>
struct Node
{
	T data;
	Node<T>* pNextNode;

	Node(T t) : data{ t }, pNextNode{ NULL } {};
};


typedef struct _tagDebugNode {

	long long pNode;
	long long pNext;
	ACTION action;

	_tagDebugNode() {};
	_tagDebugNode(long long a, long long b, ACTION _action)
		: pNode{ a }, pNext{ 0 }, action{ _action } {}

}DebugNode, * PDebugNode;

template<typename T>
class LockFreeStack
{
public:
	LockFreeStack();
	~LockFreeStack();

public:
	bool push(Node<T>* pNewNode);
	Node<T>* pop(void);

private:
	Node<T>* pTopNode;

	CircularQueue<DebugNode> queue;
};

template<typename T>
LockFreeStack<T>::LockFreeStack()
	: pTopNode{ nullptr }
{
}

template<typename T>
LockFreeStack<T>::~LockFreeStack()
{
}

// CAS �Լ� ����
bool CAS(long long* ptr, long long old_value, long long new_value) {
	// _InterlockedCompareExchange�� ptr�� old_value�� ��� new_value�� ��ü�ϰ�
	// �׷��� ������ ptr�� ���� ���� ��ȯ�մϴ�.
	return InterlockedCompareExchange64(ptr, new_value, old_value) == old_value;
}

template<typename T>
bool LockFreeStack<T>::push(Node<T>* pNewNode)
{
	Node<T>* pNode = nullptr;

	do
	{
		// top ��� ��������
		pNode = pTopNode;

		// ���ο� �߰��� ����� next�� TopNode�� pNode�� ����
		pNewNode->pNextNode = pNode;

	} while (!CAS(reinterpret_cast<long long*>(&pNode), reinterpret_cast<long long>(&pNewNode), reinterpret_cast<long long>(&pTopNode)));

	queue.enqueue(DebugNode{ reinterpret_cast<long long>(pNode), reinterpret_cast<long long>(pNewNode), ACTION::PUSH });
	 
	return true;
}

template<typename T>
Node<T>* LockFreeStack<T>::pop(void)
{
	Node<T>* pNode = nullptr;

	do
	{
		// top ��� ��������
		pNode = pTopNode;

	} while (!CAS(reinterpret_cast<long long*>(pNode), reinterpret_cast<long long>(pTopNode->pNextNode), reinterpret_cast<long long>(pTopNode)));

	queue.enqueue(DebugNode{ reinterpret_cast<long long>(pNode), reinterpret_cast<long long>(pTopNode), ACTION::POP});

	return pNode;
}

LockFreeStack<int> g_Stack;

unsigned int WINAPI Worker(void* pArg)
{
	while (1)
	{
		for (int i = 0; i < 10000; ++i)
		{
			Node<int>* pNode = new Node<int>(1);
			g_Stack.push(pNode);
		}

		for (int i = 0; i < 10000; ++i)
		{
			Node<int>* pNode = g_Stack.pop();
		}
	}
}

int main(void)
{
	HANDLE hHandle[4];

	UINT8 cnt = 2;
	for (int i = 0; i < cnt; ++i)
	{
		hHandle[i] = (HANDLE)_beginthreadex(NULL, 0, Worker, NULL, 0, NULL);
	}

	
	
	WaitForMultipleObjects(cnt, hHandle, TRUE, INFINITE);

}

