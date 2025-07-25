#ifndef AD_ENGINE_H
#define AD_ENGINE_H


#include<iostream>
#include<cassert>
#include<memory>
#include<algorithm>
#include<functional>
#include<string>
#include<sstream>
#include<fstream>
#include<filesystem>
#include<vector>
#include<stack>
#include<queue>
#include<deque>
#include<set>
#include<unordered_map>
#include<unordered_set>


#ifdef AD_ENGINE_PLATFORM_WIN32
#define VK_USE_PLATFORM_WIN32_KHR

#elif AD_ENGINE_PLATFORM_LINUX
#define VK_USE_PLATFORM_XLIB_KHR

#elif AD_ENGINE_PLATFORM_MACOS
#define VK_USE_PLATFORM_MACOS_MVK

#else
#error "Unsupported platform!"	
#endif

#define AD_ENGINE_GRAPHICS_API_VULKAN
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define __FILENAME__                    (strrchr(__FILE__, '/') + 1)
#define ENUM_TO_STR(r)                  case r: return #r
#endif // !AD_ENGINE_H
