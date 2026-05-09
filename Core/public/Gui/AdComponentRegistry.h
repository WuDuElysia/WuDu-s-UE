// AdComponentRegistry.h
#pragma once
#include "AdComponentInfo.h"
#include <unordered_map>

namespace WuDu {
	class AdEntity;

	class AdComponentRegistry {
	public:
		static AdComponentRegistry& GetInstance();

		// 注册组件类型
		void RegisterComponent(const AdComponentInfo& info);

		// 查询实体拥有的已注册组件
		std::vector<const AdComponentInfo*> GetComponentsForEntity(
			entt::registry& registry, entt::entity entity) const;

		// 查询实体未拥有的已注册组件（用于 Add Component 下拉列表）
		std::vector<const AdComponentInfo*> GetMissingComponentsForEntity(
			entt::registry& registry, entt::entity entity) const;

		// 获取所有已注册组件信息
		const std::unordered_map<uint32_t, AdComponentInfo>& GetAllRegistered() const;

	private:
		AdComponentRegistry() = default;
		std::unordered_map<uint32_t, AdComponentInfo> mRegistry;
	};
}
