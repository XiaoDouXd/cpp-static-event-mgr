#include "staticEventMgr.hpp"

namespace XD::Event
{
  void StaticEventMgr::update()
  {
    const auto startTime = clock();

    auto isEmpty = false;
    while (!isEmpty && clock() - startTime <= MAX_WAIT_TIME)
    {
      std::lock_guard<std::recursive_mutex> lock(_inst->mtx);
      isEmpty = _inst->waitingQueue.empty();
      if (isEmpty) return;

      _inst->waitingQueue.front().invoke();
      const auto event = _inst->waitingQueue.front().event;
      if (event) event->waiting.pop();
      _inst->waitingQueue.pop_front();
    }
  }

  [[maybe_unused]] void StaticEventMgr::unregisterEvent(const std::size_t &hashCode, const uuids::uuid &obj)
  {
    std::lock_guard<std::recursive_mutex> lock(_inst->mtx);

    auto &eDic = _inst->staticEvents;
    if (eDic.find(hashCode) == eDic.end()) return;
    auto &lDic = eDic[hashCode];
    if (lDic.find(obj) == lDic.end()) return;
    auto eH = lDic.find(obj);

    while (!eH->second.waiting.empty())
    {
      _inst->waitingQueue.erase(eH->second.waiting.front());
      eH->second.waiting.pop();
    }

    lDic.erase(eH);
  }

  [[maybe_unused]] void StaticEventMgr::unregisterEvent(const std::optional<std::size_t> &hashCodeOpt, const uuids::uuid &obj)
  {
    std::lock_guard<std::recursive_mutex> lock(_inst->mtx);

    if (!hashCodeOpt) return;
    const auto &hashCode = hashCodeOpt.value();
    auto &eDic = _inst->staticEvents;
    if (eDic.find(hashCode) == eDic.end()) return;
    auto &lDic = eDic[hashCode];
    if (lDic.find(obj) == lDic.end()) return;
    auto eH = lDic.find(obj);

    while (!eH->second.waiting.empty())
    {
      _inst->waitingQueue.erase(eH->second.waiting.front());
      eH->second.waiting.pop();
    }

    lDic.erase(eH);
  }

  void StaticEventMgr::reset()
  {
    _inst.reset();
    _inst = std::make_unique<StaticEventMgr::EventMgrData>();
  }
} // namespace XD::Event