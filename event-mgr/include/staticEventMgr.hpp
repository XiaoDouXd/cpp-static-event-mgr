#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <list>
#include <mutex>

#include "../3rd/uuid.h"

namespace XD::Event::UUID
{
  uuids::uuid gen();
} // namespace XD::Event::UUID

namespace XD::Event
{
  /// @brief 事件类基
  /// @example class OnMouseClick : public EventTypeBase<OnMouseClick, uint8_t> {};
  /// @tparam EType 事件类本身
  /// @tparam ...ArgTypes 事件类的变量类型
  template<class EType, typename... ArgTypes>
  class EventTypeBase
  {
  public:
    using _xd_eType = EventTypeBase<EType, ArgTypes...>;
    using _xd_fType = std::function<void(ArgTypes...)>;
    using _xd_isEventType = std::true_type;
  };

  class _xd_staticEvent_BaseFunc { public: virtual ~_xd_staticEvent_BaseFunc() = default; };
  template<class EType>
  class _xd_staticEvent_Func : public _xd_staticEvent_BaseFunc
  {
  public:
    explicit _xd_staticEvent_Func(typename EType::_xd_fType func) : func(func) {}
    typename EType::_xd_fType func;
  };

  class StaticEventMgr
  {
  private:
    /// @brief 可调用对象
    struct EventHandler;
    /// @brief 异步的可调用对象
    struct EventAsyncHandler;

  private:
    struct EventHandler
    {
    public:
      EventHandler(const uuids::uuid& objId, std::unique_ptr<_xd_staticEvent_BaseFunc>&& cb)
        :objId(objId), cb(std::move(cb)), waiting(std::queue<std::list<EventAsyncHandler>::const_iterator>()) {}
      uuids::uuid objId = uuids::uuid();
      std::unique_ptr<_xd_staticEvent_BaseFunc> cb = nullptr;
      std::queue<std::list<EventAsyncHandler>::const_iterator> waiting;
    };

    struct EventAsyncHandler
    {
    public:
      EventAsyncHandler(EventHandler* event, std::function<void()> invoke)
        :event(event), invoke(std::move(invoke)) {}
      EventHandler* event = nullptr;
      std::function<void()> invoke;
    };

    // 管理器数据
    class EventMgrData
    {
    public:
      /// @brief mutex
      std::recursive_mutex mtx;
      /// @brief 静态事件 <事件id, <事件监听成员, EventHandler>>
      std::unordered_map<std::size_t, std::map<uuids::uuid, EventHandler>> staticEvents;
      /// @brief 异步事件的等待队列
      std::list<EventAsyncHandler> waitingQueue;
    };
    std::unique_ptr<EventMgrData> _inst = std::unique_ptr<EventMgrData>();

  public:
    /// @brief 注册事件 每个 obj 只能注册一次相同事件 多余的注册会被略过
    /// @tparam EType 注册的事件类型
    /// @param obj 监听器的 id (一般用对象内存地址描述)
    /// @param cb 事件的回调
    /// @return 注册到事件的哈希值包 (可以使用这个哈希值注销事件)
    template<class EType>
      requires EType::_xd_isEventType::value&& std::is_base_of<typename EType::_xd_eType, EType>::value
    std::optional<std::size_t> registerEvent(const uuids::uuid& obj, typename EType::_xd_fType cb)
    {
      std::size_t hashCode = typeid(EType).hash_code();
      auto& eDic = _inst->staticEvents;
      if (eDic.find(hashCode) == eDic.end())
        eDic.insert({ hashCode, std::map<uuids::uuid, EventHandler>() });
      auto& lDic = eDic[hashCode];
      if (lDic.find(obj) != lDic.end()) return std::nullopt;
      lDic.insert(std::pair(obj, EventHandler(
        obj,
        std::unique_ptr<_xd_staticEvent_BaseFunc>
        (reinterpret_cast<_xd_staticEvent_BaseFunc*>(new _xd_staticEvent_Func<EType>(cb)))
      )));
      return std::make_optional<std::size_t>(hashCode);
    }

    /// @brief 注销事件
    /// @tparam EType 注销的事件类型
    /// @param obj 监听器的 id (一般用对象内存地址描述)
    template<class EType>
      requires EType::_xd_isEventType::value&& std::is_base_of<typename EType::_xd_eType, EType>::value
    std::optional<std::size_t> unregisterEvent(const uuids::uuid& obj)
    {
      std::size_t hashCode = typeid(EType).hash_code();
      auto& eDic = _inst->staticEvents;
      if (eDic.find(hashCode) == eDic.end()) return std::nullopt;
      auto& lDic = eDic[hashCode];
      if (lDic.find(obj) == lDic.end()) return std::nullopt;
      auto eH = lDic.find(obj);

      while (!eH->second.waiting.empty())
      {
        _inst->waitingQueue.erase(eH->second.waiting.front());
        eH->second.waiting.pop();
      }

      lDic.erase(eH);
      return std::make_optional<std::size_t>(hashCode);
    }

    /// @brief 注销事件
    /// @param hashCode 事件的哈希值
    /// @param obj 监听器的 id (一般用对象内存地址描述)
    void unregisterEvent(const std::size_t& hashCode, const uuids::uuid& obj);

    /// @brief 注销事件
    /// @param hashCodeOpt 事件的哈希值包
    /// @param obj 监听器的 id (一般用对象内存地址描述)
    void unregisterEvent(const std::optional<std::size_t>& hashCodeOpt, const uuids::uuid& obj);

    /// @brief 某事件的所有监听
    /// @tparam EType 事件类型
    template<class EType>
      requires EType::_xd_isEventType::value&& std::is_base_of<typename EType::_xd_eType, EType>::value
    void clearEvent()
    {
      std::lock_guard<std::recursive_mutex> lock(_inst->mtx);

      // 清空所有事件
      std::size_t hashCode = typeid(EType).hash_code();
      auto& eDic = _inst->staticEvents;
      if (eDic.find(hashCode) == eDic.end()) return;
      for (auto& lDic : eDic[hashCode])
      {
        while (!lDic.second.waiting.empty())
        {
          _inst->waitingQueue.erase(lDic.second.waiting.front());
          lDic.second.waiting.pop();
        }
      }
      eDic.erase(hashCode);
    }

    /// @brief 广播
    /// @tparam EType 事件类型
    /// @tparam ...ArgTypes 参数包 (定义事件类型时所指定的参数)
    /// @param ...args 要传递参数
    template<class EType, typename... ArgTypes>
      requires EType::_xd_isEventType::value&& std::is_base_of<typename EType::_xd_eType, EType>::value
    && std::is_same<typename EType::_xd_fType, std::function<void(ArgTypes...)>>::value
      void broadcast(ArgTypes... args)
    {
      std::lock_guard<std::recursive_mutex> lock(_inst->mtx);

      // 同步广播直接调用, 不创建任何闭包
      std::size_t hashCode = typeid(EType).hash_code();
      auto& eDic = _inst->staticEvents;
      if (eDic.find(hashCode) == eDic.end()) return;
      for (auto& lDic : eDic[hashCode])
      {
        reinterpret_cast<_xd_staticEvent_Func<EType>*>(lDic.second.cb.get())->
          func(std::forward<ArgTypes>(args)...);
      }
    }

    /// @brief 异步地广播
    /// @tparam EType 事件类型
    /// @tparam ...ArgTypes 参数包 (定义事件类型时所指定的参数)
    /// @param ...args 要传递的参数
    template<class EType, typename... ArgTypes>
      requires EType::_xd_isEventType::value&& std::is_base_of<typename EType::_xd_eType, EType>::value
    && std::is_same<typename EType::_xd_fType, std::function<void(ArgTypes...)>>::value
      void broadcastAsync(ArgTypes... args)
    {
      std::lock_guard<std::recursive_mutex> lock(_inst->mtx);

      // 异步广播需要构造闭包, 这里直接用 lambda 偷个懒
      std::size_t hashCode = typeid(EType).hash_code();
      auto& eDic = _inst->staticEvents;
      if (eDic.find(hashCode) == eDic.end()) return;
      for (auto& lDic : eDic[hashCode])
      {
        // 回调指针
        auto cbPtr = reinterpret_cast<_xd_staticEvent_Func<EType>*>(lDic.second.cb.get());

        // 使用 lambda 构造闭包
        _inst->waitingQueue.emplace_back(
          EventAsyncHandler(
            &lDic.second,
            [cbPtr, args...]() {cbPtr->func(args...); }
          ));
        lDic.second.waiting.emplace(--(_inst->waitingQueue.end()));
      }
    }

    /// @brief 异步地广播 (带广播结束回调)
    /// @tparam EType 事件类型
    /// @tparam ...ArgTypes 参数包 (定义事件类型时所指定的参数)
    /// @param ...args 要传递的参数
    template<class EType, typename... ArgTypes>
    requires EType::_xd_isEventType::value&& std::is_base_of<typename EType::_xd_eType, EType>::value
      && std::is_same<typename EType::_xd_fType, std::function<void(ArgTypes...)>>::value
      void broadcastAsyncWithCallback(std::function<void(ArgTypes...)> finishCallback, ArgTypes... args)
    {
      std::lock_guard<std::recursive_mutex> lock(_inst->mtx);

      // 异步广播需要构造闭包, 这里直接用 lambda 偷个懒
      std::size_t hashCode = typeid(EType).hash_code();
      auto& eDic = _inst->staticEvents;
      if (eDic.find(hashCode) == eDic.end()) return;
      for (auto& lDic : eDic[hashCode])
      {
        // 回调指针
        auto cbPtr = reinterpret_cast<_xd_staticEvent_Func<EType>*>(lDic.second.cb.get());

        // 使用 lambda 构造闭包
        _inst->waitingQueue.emplace_back(
          EventAsyncHandler(
            &lDic.second,
            [cbPtr, args...]() {cbPtr->func(args...); }
        ));
        lDic.second.waiting.emplace(--(_inst->waitingQueue.end()));
      }

      // 该次异步广播中所有函数调用完成回调, 可用于清垃圾
      _inst->waitingQueue.emplace_back(
        EventAsyncHandler(
          nullptr,
          [finishCallback, args...]() {finishCallback(args...); }
      ));
    }

  public:

    /// @brief 初始化
    void init();

    /// @brief 刷新帧
    void update();

    /// @brief 销毁
    void destroy();
  };
} // namespace XD::Event