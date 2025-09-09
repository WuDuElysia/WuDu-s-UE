// AdEvent.h
#pragma once
#include <chrono>
#include <string>

namespace ade {
        enum class EventType {
                MouseClick,
                MouseMove,
                MouseRelease,
                MouseScroll,
                KeyPress,
                KeyRelease,
                KeyRepeat,
                WindowResize,
                FramebufferResize,
                CharInput,
                Unknown
        };

        class Event {
        public:
                Event(EventType type)
                        : m_Type(type)
                        , m_Timestamp(std::chrono::high_resolution_clock::now())
                        , m_Handled(false) {
                }

                virtual ~Event() = default;

                EventType GetType() const { return m_Type; }
                const std::chrono::high_resolution_clock::time_point& GetTimestamp() const { return m_Timestamp; }

                bool IsHandled() const { return m_Handled; }
                void SetHandled(bool handled) { m_Handled = handled; }

        protected:
                EventType m_Type;
                std::chrono::high_resolution_clock::time_point m_Timestamp;
                bool m_Handled;
        };

        // 鼠标事件基类
        class MouseEvent : public Event {
        public:
                MouseEvent(EventType type, float x, float y)
                        : Event(type), m_X(x), m_Y(y) {
                }

                float GetX() const { return m_X; }
                float GetY() const { return m_Y; }

        protected:
                float m_X, m_Y;
        };

        // 鼠标点击事件
        class MouseClickEvent : public MouseEvent {
        public:
                MouseClickEvent(float x, float y, int button, int mods)
                        : MouseEvent(EventType::MouseClick, x, y)
                        , m_Button(button)
                        , m_Mods(mods) {
                }

                int GetButton() const { return m_Button; }
                int GetMods() const { return m_Mods; }

                static EventType StaticType() { return EventType::MouseClick; }

        private:
                int m_Button, m_Mods;
        };

        // 鼠标移动事件
        class MouseMoveEvent : public MouseEvent {
        public:
                MouseMoveEvent(float x, float y, float deltaX, float deltaY)
                        : MouseEvent(EventType::MouseMove, x, y)
                        , m_DeltaX(deltaX)
                        , m_DeltaY(deltaY) {
                }

                float GetDeltaX() const { return m_DeltaX; }
                float GetDeltaY() const { return m_DeltaY; }

                static EventType StaticType() { return EventType::MouseMove; }

        private:
                float m_DeltaX, m_DeltaY;
        };

        // 鼠标释放事件
        class MouseReleaseEvent : public MouseEvent {
        public:
                MouseReleaseEvent(float x, float y, int button, int mods)
                        : MouseEvent(EventType::MouseRelease, x, y)
                        , m_Button(button)
                        , m_Mods(mods) {
                }

                int GetButton() const { return m_Button; }
                int GetMods() const { return m_Mods; }

                static EventType StaticType() { return EventType::MouseRelease; }

        private:
                int m_Button, m_Mods;
        };

        // 鼠标滚动事件
        class MouseScrollEvent : public Event {
        public:
                MouseScrollEvent(float xOffset, float yOffset)
                        : Event(EventType::MouseScroll)
                        , m_XOffset(xOffset)
                        , m_YOffset(yOffset) {
                }

                float GetXOffset() const { return m_XOffset; }
                float GetYOffset() const { return m_YOffset; }

                static EventType StaticType() { return EventType::MouseScroll; }

        private:
                float m_XOffset, m_YOffset;
        };

        // 键盘事件基类
        class KeyEvent : public Event {
        public:
                KeyEvent(EventType type, int key, int mods)
                        : Event(type)
                        , m_Key(key)
                        , m_Mods(mods) {
                }

                int GetKey() const { return m_Key; }
                int GetMods() const { return m_Mods; }

        protected:
                int m_Key, m_Mods;
        };

        // 按键按下事件
        class KeyPressEvent : public KeyEvent {
        public:
                KeyPressEvent(int key, int mods) : KeyEvent(EventType::KeyPress, key, mods) {}
                static EventType StaticType() { return EventType::KeyPress; }
        };

        // 按键释放事件
        class KeyReleaseEvent : public KeyEvent {
        public:
                KeyReleaseEvent(int key, int mods) : KeyEvent(EventType::KeyRelease, key, mods) {}
                static EventType StaticType() { return EventType::KeyRelease; }
        };

        // 按键重复事件
        class KeyRepeatEvent : public KeyEvent {
        public:
                KeyRepeatEvent(int key, int mods) : KeyEvent(EventType::KeyRepeat, key, mods) {}
                static EventType StaticType() { return EventType::KeyRepeat; }
        };

        // 窗口大小改变事件
        class WindowResizeEvent : public Event {
        public:
                WindowResizeEvent(int width, int height)
                        : Event(EventType::WindowResize)
                        , m_Width(width)
                        , m_Height(height) {
                }

                int GetWidth() const { return m_Width; }
                int GetHeight() const { return m_Height; }

                static EventType StaticType() { return EventType::WindowResize; }

        private:
                int m_Width, m_Height;
        };

        // 帧缓冲区大小改变事件
        class FramebufferResizeEvent : public Event {
        public:
                FramebufferResizeEvent(int width, int height)
                        : Event(EventType::FramebufferResize)
                        , m_Width(width)
                        , m_Height(height) {
                }

                int GetWidth() const { return m_Width; }
                int GetHeight() const { return m_Height; }

                static EventType StaticType() { return EventType::FramebufferResize; }

        private:
                int m_Width, m_Height;
        };

        // 字符输入事件
        class CharInputEvent : public Event {
        public:
                CharInputEvent(unsigned int codepoint)
                        : Event(EventType::CharInput)
                        , m_Codepoint(codepoint) {
                }

                unsigned int GetCodepoint() const { return m_Codepoint; }
                char GetChar() const { return static_cast<char>(m_Codepoint); }

                static EventType StaticType() { return EventType::CharInput; }

        private:
                unsigned int m_Codepoint;
        };
}