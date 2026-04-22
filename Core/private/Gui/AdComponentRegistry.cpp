#define NOMINMAX
#include "Gui/AdComponentRegistry.h"
#include "Adlog.h"

namespace WuDu {

	AdComponentRegistry& AdComponentRegistry::GetInstance() {
		static AdComponentRegistry instance;
		return instance;
	}

	void AdComponentRegistry::RegisterComponent(const AdComponentInfo& info) {
		auto it = mRegistry.find(info.typeId);
		if (it != mRegistry.end()) {
			LOG_W("AdComponentRegistry: Overwriting existing registration for typeId {0} ({1})",
				info.typeId, info.displayName);
		}
		mRegistry[info.typeId] = info;
	}

	std::vector<const AdComponentInfo*> AdComponentRegistry::GetComponentsForEntity(
		entt::registry& registry, entt::entity entity) const {
		std::vector<const AdComponentInfo*> result;
		for (const auto& [typeId, info] : mRegistry) {
			if (info.hasComponent && info.hasComponent(registry, entity)) {
				result.push_back(&info);
			}
		}
		return result;
	}

	std::vector<const AdComponentInfo*> AdComponentRegistry::GetMissingComponentsForEntity(
		entt::registry& registry, entt::entity entity) const {
		std::vector<const AdComponentInfo*> result;
		for (const auto& [typeId, info] : mRegistry) {
			if (info.hasComponent && !info.hasComponent(registry, entity)) {
				result.push_back(&info);
			}
		}
		return result;
	}

	const std::unordered_map<uint32_t, AdComponentInfo>& AdComponentRegistry::GetAllRegistered() const {
		return mRegistry;
	}

}
