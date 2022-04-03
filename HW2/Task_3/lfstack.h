#pragma once

#include <atomic>

//#include <util/generic/noncopyable.h>
//#include <util/system/atomic.h>

// lock free lifo stack
template <class T>
class TLockFreeStack {
    struct TNode {
        T Value;
        std::atomic<TNode*> Next;

        TNode() = default;

        template <class U>
        explicit TNode(U&& val)
                : Value(std::forward<U>(val))
                , Next(nullptr)
        {
        }
    };

    std::atomic<TNode*> Head;
    std::atomic<TNode*> FreePtr;
    std::atomic_int DequeueCount;

    void TryToFreeMemory() {
        TNode* current = FreePtr.load();
        if (!current)
            return;
        if (DequeueCount.fetch_add(0) == 1) {
            // node current is in free list, we are the last thread so try to cleanup
            if (FreePtr.compare_exchange_strong(current, (TNode*)nullptr))
                EraseList(current);
        }
    }
    void EraseList(TNode* volatile p) {
        while (p) {
            TNode* next = p->Next;
            delete p;
            p = next;
        }
    }
    void EnqueueImpl(TNode* volatile head, TNode* volatile tail) {
        for (;;) {
            tail->Next = Head.load();
            if (Head.compare_exchange_strong(tail->Next, head))
                break;
        }
    }
    template <class U>
    void EnqueueImpl(U&& u) {
        TNode* volatile node = new TNode(std::forward<U>(u));
        EnqueueImpl(node, node);
    }

public:
    TLockFreeStack()
            : Head(nullptr)
            , FreePtr(nullptr)
            , DequeueCount(0)
    {
    }
    ~TLockFreeStack() {
        EraseList(Head);
        EraseList(FreePtr);
    }

    void Enqueue(const T& t) {
        EnqueueImpl(t);
    }

    void Enqueue(T&& t) {
        EnqueueImpl(std::move(t));
    }

    template <typename TCollection>
    void EnqueueAll(const TCollection& data) {
        EnqueueAll(data.begin(), data.end());
    }
    template <typename TIter>
    void EnqueueAll(TIter dataBegin, TIter dataEnd) {
        if (dataBegin == dataEnd) {
            return;
        }
        TIter i = dataBegin;
        TNode* volatile node = new TNode(*i);
        TNode* volatile tail = node;

        for (++i; i != dataEnd; ++i) {
            TNode* nextNode = node;
            node = new TNode(*i);
            node->Next = nextNode;
        }
        EnqueueImpl(node, tail);
    }
    bool Dequeue(T* res) {
        DequeueCount++;
        for (TNode* current = Head.load(); current; current = Head.load()) {
            if (Head.compare_exchange_strong(current, current->Next.load())) {
                *res = std::move(current->Value);
                // delete current; // ABA problem
                // even more complex node deletion
                TryToFreeMemory();
                if (DequeueCount.fetch_add(-1) == 0) {
                    // no other Dequeue()s, can safely reclaim memory
                    delete current;
                } else {
                    // Dequeue()s in progress, put node to free list
                    for (;;) {
                        current->Next.store(FreePtr.load());
                        if (FreePtr.compare_exchange_strong(current->Next, current))
                            break;
                    }
                }
                return true;
            }
        }
        TryToFreeMemory();
        DequeueCount--;
        return false;
    }
    // add all elements to *res
    // elements are returned in order of dequeue (top to bottom; see example in unittest)
    template <typename TCollection>
    void DequeueAll(TCollection* res) {
        DequeueCount++;
        for (TNode* current = Head.load(); current; current = Head.load()) {
            if (Head.compare_exchange_strong(current, (TNode*)nullptr)) {
                for (TNode* x = current; x;) {
                    res->push_back(std::move(x->Value));
                    x = x->Next;
                }
                // EraseList(current); // ABA problem
                // even more complex node deletion
                TryToFreeMemory();
                if (DequeueCount.fetch_add(-1) == 0) {
                    // no other Dequeue()s, can safely reclaim memory
                    EraseList(current);
                } else {
                    // Dequeue()s in progress, add nodes list to free list
                    TNode* currentLast = current;
                    while (currentLast->Next) {
                        currentLast = currentLast->Next;
                    }
                    for (;;) {
                        currentLast->Next.store(FreePtr.load());
                        if (FreePtr.compare_exchange_strong(currentLast->Next, current))
                            break;
                    }
                }
                return;
            }
        }
        TryToFreeMemory();
        DequeueCount--;
    }
    bool DequeueSingleConsumer(T* res) {
        for (TNode* current = Head.load(); current; current = Head.load()) {
            if (Head.compare_exchange_strong(current, current->Next)) {
                *res = std::move(current->Value);
                delete current; // with single consumer thread ABA does not happen
                return true;
            }
        }
        return false;
    }
    // add all elements to *res
    // elements are returned in order of dequeue (top to bottom; see example in unittest)
    template <typename TCollection>
    void DequeueAllSingleConsumer(TCollection* res) {
        for (TNode* current = Head.load(); current; current = Head.load()) {
            if (Head.compare_exchange_strong(current, (TNode*)nullptr)) {
                for (TNode* x = current; x;) {
                    res->push_back(std::move(x->Value));
                    x = x->Next;
                }
                EraseList(current); // with single consumer thread ABA does not happen
                return;
            }
        }
    }

    bool IsEmpty() {
        DequeueCount.fetch_add(0);        // mem barrier
        return Head.load() == nullptr; // without lock, so result is approximate
    }
};