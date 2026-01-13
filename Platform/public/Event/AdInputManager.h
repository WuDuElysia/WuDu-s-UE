
#pragma once
#include"AdEngine.h"
#include"AdEvent.h"

namespace WuDu {
	class InputManager {
	public:
		static InputManager& GetInstance() {
			static InputManager instance;
			return instance;
		}

		// 模板函数，将事件添加到事件队列中
		template<typename T, typename... Args>
		void QueueEvent(Args&&... args) {
			auto event = std::make_unique<T>(std::forward<Args>(args)...);
			m_EventQueue.push(std::move(event));
		}

		// 处理事件队列，遍历队列中的事件
		void ProcessEvents() {
			while (!m_EventQueue.empty()) {
				auto& event = m_EventQueue.front();

				// 根据类型查找订阅者
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
			auto wrapper = [callback](WuDu::Event& e) {
				callback(static_cast<T&>(e));
				};

			m_Subscribers[T::StaticType()].push_back(wrapper);
		}

		// 清除订阅者
		void ClearSubscribers() {
			m_Subscribers.clear();
		}

	private:
		InputManager() = default;
		~InputManager() = default;

		std::queue<std::unique_ptr<WuDu::Event>> m_EventQueue;
		std::unordered_map<WuDu::EventType, std::vector<std::function<void(WuDu::Event&)>>> m_Subscribers;
	};
}