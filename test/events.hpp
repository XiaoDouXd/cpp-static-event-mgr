
#include <iostream>
#include <string>
#include "staticEventMgr.hpp"

class A {
public:
  A() { std::cout << "call constructor A" << std::endl; }
  ~A() { std::cout << "call deconstructor A" << std::endl; }
  inline const char* Func() const { return "call A::Func"; }
};

// 事件定义

// 带 int 和 double 参数的事件
class Event_IntAndDouble : public XD::Event::EventTypeBase<Event_IntAndDouble, int, double> {};

// 带 string 参数的事件
class Event_Str : public XD::Event::EventTypeBase<Event_Str, std::string> {};

// 带 A * 参数的事件
class Event_PointerA : public XD::Event::EventTypeBase<Event_PointerA, A*> {};

// 不带参事件
class Event_Void : public XD::Event::EventTypeBase<Event_Void> {};
