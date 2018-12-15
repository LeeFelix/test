#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

//by wangh
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

typedef enum {
    HIGH_PRIORITY,
    MID_PRIORITY,
    LOW_PRIORITY
} MessagePriority;

template <typename T>
class Message
{
private:
    T m_id;
    MessagePriority m_priority;
    Message() {}
public:
    void* arg1; //be careful for its lifetime
    void* arg2;
    void* arg3;
    static Message<T>* create(T id, MessagePriority priority = MID_PRIORITY)
    {
        Message<T>* m = new Message<T>();
        if (!m)
            return nullptr;
        m->m_id = id;
        m->m_priority = priority;
        m->arg1 = NULL;
        m->arg2 = NULL;
        m->arg3 = NULL;
        return m;
    }
    inline T GetID()
    {
        return m_id;
    }
    inline MessagePriority GetPriority()
    {
        return m_priority;
    }
    bool PriorityHighThan(Message& msg)
    {
        return m_priority < msg.GetPriority();
    }
    bool PriorityLowOrEqual(Message& msg)
    {
        return m_priority >= msg.GetPriority();
    }
};

template <typename T>
class MessageQueue
{
private:
    std::mutex mtx;
    std::condition_variable cond;
    std::list<Message<T>*> msg_list;

public:
    MessageQueue() {}
    ~MessageQueue()
    {
        assert(msg_list.empty());
    }
    void Push(Message<T>* msg, bool signal = false)
    {
        std::unique_lock<std::mutex> lk(mtx);
#if 0
        if(!msg_list.empty()) {
            Message<T> &m = msg_list.back();
            if(msg.PriorityLowOrEqual(m)) {
                msg_list.push_back(msg);
                goto out;
            }
            for(std::list<Message<T>>::iterator i = msg_list.begin(); i != msg_list.end(); i++) {
                Message<T> &m = *i;
                if(msg.PriorityHighThan(m)) {
                    msg_list.insert(i, msg);
                    goto out;
                }
            }
            assert(0);
        } else {
            msg_list.push_back(msg);
        }
#else
        msg_list.push_back(msg);
#endif
out:
        if (signal) {
            cond.notify_one();
        }
    }
    Message<T>* PopFront()
    {
        std::unique_lock<std::mutex> lk(mtx);
        assert(!msg_list.empty());
        Message<T>* msg = msg_list.front();
        msg_list.pop_front();
        return msg;
    }
    bool Empty()
    {
        std::unique_lock<std::mutex> lk(mtx);
        return msg_list.empty();
    }
    int size()
    {
        std::unique_lock<std::mutex> lk(mtx);
        return msg_list.size();
    }
    void WaitMessage()
    {
        std::unique_lock<std::mutex> lk(mtx);
        cond.wait(lk);
    }
    void clear()
    {
        std::unique_lock<std::mutex> lk(mtx);
        for (typename std::list<Message<T>*>::iterator i = msg_list.begin(); i != msg_list.end(); i++) {
            Message<T>* m = *i;
            delete m;
        }
        msg_list.clear();
    }
};

#endif // MESSAGEQUEUE_H
