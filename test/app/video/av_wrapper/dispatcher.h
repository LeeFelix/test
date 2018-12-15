// by wangh
#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "video_common.h"

template <class T, typename DATA>
class Dispatcher  // DataDispatcher
{
 protected:
  std::mutex list_mutex;
  std::list<T*> handlers;

 public:
  Dispatcher() {}
  virtual ~Dispatcher() { assert(handlers.size() == 0); }
  void AddHandler(T* handler) {
    std::lock_guard<std::mutex> lk(list_mutex);
    UNUSED(lk);
    handlers.push_back(handler);
  }
  void RemoveHandler(T* handler) {
    std::lock_guard<std::mutex> lk(list_mutex);
    UNUSED(lk);
    handlers.remove(handler);
  }
  virtual void Dispatch(DATA* pkt) {}
  bool Empty() {
    std::lock_guard<std::mutex> lk(list_mutex);
    return handlers.empty();
  }
};

#endif  // DISPATCHER_H
