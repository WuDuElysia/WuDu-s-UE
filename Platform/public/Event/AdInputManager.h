
#pragma once
#include"AdEngine.h"
#include"AdEvent.h"

namespace ade {
        class InputManager {
        public:
                static InputManager& GetInstance() {
                        static InputManager instance;
                        return instance;
                }

                // 模板方法：队列事件（由事件适配器调用）
                template<typename T, typename... Args>
                void QueueEvent(Args&&... args) {
                        auto event = std::make_unique<T>(std::forward<Args>(args)...);
                        m_EventQueue.push(std::move(event));
                }

                // 处理事件队列（在主循环中调用）
                void ProcessEvents() {
                        while (!m_EventQueue.empty()) {
                                auto& event = m_EventQueue.front();

                                // 分发给所有订阅者
                                auto it = m_Subscribers.find(event->GetType());
                                if (it != m_Subscribers.end()) {
                                        for (auto& callback : (it->second)) {
                                                if (!event->IsHandled()) {
                                                        callback(*event);
                                                }
                                        }
                                }

                                m_EventQueue.pop();
                        }
                }

                // 订阅事件
                template<typename T>
                void Subscribe(std::function<void(T&)> callback) {
                        auto wrapper = [callback](ade::Event& e) {
                                callback(static_cast<T&>(e));
                                };

                        m_Subscribers[T::StaticType()].push_back(wrapper);
                }

                // 清除所有订阅
                void ClearSubscribers() {
                        m_Subscribers.clear();
                }

        private:
                InputManager() = default;
                ~InputManager() = default;

                std::queue<std::unique_ptr<ade::Event>> m_EventQueue;
                std::unordered_map<ade::EventType, std::vector<std::function<void(ade::Event&)>>> m_Subscribers;
        };
}