// AdComponentInfo.h
#pragma once
#include "AdPropertyDescriptor.h"
#include "entt/entity/registry.hpp"
#include <vector>
#include <cstdint>

namespace WuDu {

	// 组件元信息 — 存储一个组件类型的注册信息
	struct AdComponentInfo {
		uint32_t typeId;                            // EnTT type_id hash
		std::string displayName;                    // 显示名称
		std::vector<AdPropertyDescriptor> properties; // 属性列表

		// 检查实体是否拥有此组件的函数
		std::function<bool(entt::registry&, entt::entity)> hasComponent;
		// 获取组件 void* 指针的函数
		std::function<void*(entt::registry&, entt::entity)> getComponent;
		// 为实体添加此组件的函数
		std::function<void(entt::registry&, entt::entity)> addComponent;
		// 从实体移除此组件的函数
		std::function<void(entt::registry&, entt::entity)> removeComponent;
	};
}
