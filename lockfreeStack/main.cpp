
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
	Node<T>* pNext;

	Node(T t) : data{ t }, pNext{ NULL } {};
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
	bool pop(T& value);

private:
	Node<T>* pTop;

	CircularQueue<DebugNode> queue;
};

template<typename T>
LockFreeStack<T>::LockFreeStack()
	: pTop{ nullptr }
{
}

template<typename T>
LockFreeStack<T>::~LockFreeStack()
{
	while (pTop) {
		Node<T>* temp = pTop;
		pTop = pTop->pNext;
		delete temp;
	}
}

// CAS �Լ� ����
template<typename T>
bool CAS(Node<T>** ptr, Node<T>* old_value, Node<T>* new_value) {
	// InterlockedExchangePointer�� ����Ͽ� ������ ���� ���������� ��ü
	// _InterlockedCompareExchange�� ptr�� old_value�� ��� new_value�� ��ü�ϰ�
	// �׷��� ������ ptr�� ���� ���� ��ȯ�մϴ�.
	return InterlockedCompareExchange64(
		reinterpret_cast<LONG64*>(ptr),                  // ��� ������ (ptr)
		reinterpret_cast<LONG64>(new_value),            // �� ��
		reinterpret_cast<LONG64>(old_value)             // ���� ��
	) == reinterpret_cast<LONG64>(old_value);           // ���� ���� ��ȯ
}

template<typename T>
bool LockFreeStack<T>::push(Node<T>* pNewNode)
{
	Node<T>* pNode = nullptr;

	do
	{
		// top ��� ��������
		pNode = pTop;

		// ���ο� �߰��� ����� next�� TopNode�� pNode�� ����
		pNewNode->pNext = pNode;

	} while (!CAS(&pTop, pNode, pNewNode));

	queue.enqueue(DebugNode{ reinterpret_cast<long long>(pNode), reinterpret_cast<long long>(pNewNode), ACTION::PUSH });

	return true;
}

template<typename T>
bool LockFreeStack<T>::pop(T& value)
{
	Node<T>* pOldTop = nullptr;
	Node<T>* pNext = nullptr;

	do
	{
		// top ��� ��������
		pOldTop = pTop;

		if (pOldTop == NULL)
			return false;

		pNext = pOldTop->pNext;

	} while (!CAS(&pTop, pOldTop, pNext));

	queue.enqueue(DebugNode{ reinterpret_cast<long long>(pOldTop), reinterpret_cast<long long>(pTop), ACTION::POP});
	value = pOldTop->data;
	delete pOldTop;

	return true;
}

LockFreeStack<int> g_Stack;

unsigned int WINAPI Worker(void* pArg)
{
	int cnt = rand() % 1000;

	while (1)
	{

		for (int i = 0; i < cnt; ++i)
		{
			Node<int>* pNode = new Node<int>(i);
			g_Stack.push(pNode);
		}

		int value;
		for (int i = 0; i < cnt; ++i)
		{
			if (!g_Stack.pop(value))
				DebugBreak();
		}
	}
}

int main(void)
{
	const int ThreadCnt = 4;
	HANDLE hHandle[ThreadCnt];

	for (int i = 0; i < ThreadCnt; ++i)
	{
		hHandle[i] = (HANDLE)_beginthreadex(NULL, 0, Worker, NULL, 0, NULL);
	}

	WaitForMultipleObjects(ThreadCnt, hHandle, TRUE, INFINITE);
}

//
//thread_local int tl_id;
//
//int Thread_id()
//{
//	return tl_id;
//}
//
////typedef CQUEUE	MY_QUEUE;
//typedef LFQUEUE	MY_QUEUE;
//
//// �� 1õ�� ���̳�, queue�� ��� O(1)�̱� ������ �ӵ��� ������.
//constexpr int LOOP = 1000'0000;
//
//void worker(MY_QUEUE* queue, int threadNum, int thread_id)
//{
//	tl_id = thread_id;
//	for (int i = 0; i < LOOP / threadNum; i++) {
//		// ¦���� enq, Ȧ���� deq�� ó���Ѵ�.
//		// �����Ͱ� 128�� ���ϸ� deq�� ���� ����. �̴� queue�� empty �� ��츦 �ּ�ȭ�ϱ� ���ؼ� �ϴ� �۾��̴�.
//		if ((rand() % 2) || i < 8 / threadNum) {
//			queue->ENQ(i);
//		}
//		else {
//			queue->DEQ(i);
//		}
//	}
//}
//
//int main()
//{
//	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
//		vector <thread> threads;
//		MY_QUEUE my_queue;
//		auto start_t = high_resolution_clock::now();
//		for (int i = 0; i < num_threads; ++i)
//			threads.emplace_back(worker, &my_queue, num_threads, i);
//		for (auto& th : threads)
//			th.join();
//		auto end_t = high_resolution_clock::now();
//		auto exec_t = end_t - start_t;
//		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
//		my_queue.print20();
//		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
//	}
//}