
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

// CAS 함수 정의
template<typename T>
bool CAS(Node<T>** ptr, Node<T>* old_value, Node<T>* new_value) {
	// InterlockedExchangePointer를 사용하여 포인터 값을 원자적으로 교체
	// _InterlockedCompareExchange는 ptr이 old_value일 경우 new_value로 교체하고
	// 그렇지 않으면 ptr의 기존 값을 반환합니다.
	return InterlockedCompareExchange64(
		reinterpret_cast<LONG64*>(ptr),                  // 대상 포인터 (ptr)
		reinterpret_cast<LONG64>(new_value),            // 새 값
		reinterpret_cast<LONG64>(old_value)             // 예상 값
	) == reinterpret_cast<LONG64>(old_value);           // 성공 여부 반환
}

template<typename T>
bool LockFreeStack<T>::push(Node<T>* pNewNode)
{
	Node<T>* pNode = nullptr;

	do
	{
		// top 노드 가져오기
		pNode = pTop;

		// 새로운 추가할 노드의 next를 TopNode인 pNode로 설정
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
		// top 노드 가져오기
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
//// 왜 1천만 번이냐, queue의 경우 O(1)이기 때문에 속도가 빠르다.
//constexpr int LOOP = 1000'0000;
//
//void worker(MY_QUEUE* queue, int threadNum, int thread_id)
//{
//	tl_id = thread_id;
//	for (int i = 0; i < LOOP / threadNum; i++) {
//		// 짝수면 enq, 홀수면 deq를 처리한다.
//		// 데이터가 128개 이하면 deq를 하지 말자. 이는 queue가 empty 일 경우를 최소화하기 위해서 하는 작업이다.
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