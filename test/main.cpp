#include "events.hpp"
#include <iostream>

void FuncA1(int a, double b) {
  std::cout << "call FuncA1 with param: a = " << a << ", b = " << b << std::endl;
}

void FuncA2(int a, double b) {
  std::cout << "call FuncA2 with param: a = " << a << ", b = " << b << std::endl;
}

void FuncB(const std::string& s) {
  std::cout << "call FuncB with param: s = `" << s << "`" << std::endl;
}

void FuncC() {
  std::cout << "call FuncC" << std::endl;
}

void FuncD(A* a) {
  std::cout << "call FuncD with param: a = " << (ptrdiff_t)a << ", a->Func() = `" << a->Func() << "`" << std::endl;
}

int main() {
  auto mgr = XD::Event::StaticEventMgr();

  // init event manager
  mgr.init();

  // register event
  const auto uuid = XD::Event::UUID::gen();
  const auto uuid2 = XD::Event::UUID::gen();
  const auto handlerA1 = mgr.registerEvent<Event_IntAndDouble>(uuid, FuncA1);
  const auto handlerA2 = mgr.registerEvent<Event_IntAndDouble>(uuid2, FuncA2);
  const auto handlerB = mgr.registerEvent<Event_Str>(uuid, FuncB);
  const auto handlerC = mgr.registerEvent<Event_Void>(uuid, FuncC);
  const auto handlerD = mgr.registerEvent<Event_PointerA>(uuid, FuncD);

  // dispatch event
  mgr.broadcast<Event_IntAndDouble>(1, 2.2);

  // unregister event
  mgr.unregisterEvent<Event_IntAndDouble>(uuid);
  mgr.broadcast<Event_IntAndDouble>(2, 4.4);

  // dispatch async event
  mgr.broadcast<Event_Str>(std::string("aaa"));
  mgr.broadcastAsync<Event_Str>(std::string("bbb"));
  mgr.broadcast<Event_Str>(std::string("ddd"));

  // dispatch async with callback
  auto a = new A();
  mgr.broadcastAsyncWithCallback<Event_PointerA>(std::function([](A* v) {
    delete v;
  }), a);

  // unregister all
  mgr.clearEvent<Event_IntAndDouble>();
  mgr.broadcast<Event_IntAndDouble>(4, 8.8);

  // update mgr (for async broadcast)
  mgr.update();

  // reset event manager
  mgr.reset();

  return 0;
}