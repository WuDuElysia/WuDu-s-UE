// AdPropertyDescriptor.h
#pragma once
#include <string>
#include <functional>
#include <optional>
#include <vector>
#include <glm/glm.hpp>

namespace WuDu {

	// 支持的属性类型枚举
	enum class PropertyType {
		Float, Int, Bool,
		Vec2, Vec3, Vec4,
		Color3, Color4,
		Enum
	};

	// 属性描述符 — 描述组件中一个可编辑属性
	struct AdPropertyDescriptor {
		std::string name;           // 属性内部名称
		std::string displayLabel;   // 显示标签
		PropertyType type;          // 属性类型

		// 类型擦除的 getter/setter，操作 void* 指向的组件实例
		std::function<void(void* component, void* outValue)> getter;
		std::function<void(void* component, const void* inValue)> setter;

		// 数值类型的可选范围参数
		std::optional<float> minValue;
		std::optional<float> maxValue;
		float dragSpeed = 0.1f;

		// 枚举类型的选项列表
		std::vector<std::string> enumOptions;
	};
}
