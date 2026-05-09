#include "Resource/AdResourceManager.h"

namespace WuDu {

	AdResourceManager::AdResourceManager() = default;
	AdResourceManager::~AdResourceManager() = default;

	AdResourceManager* AdResourceManager::GetInstance() {
		static AdResourceManager instance;
		return &instance;
	}

	void AdResourceManager::UnloadUnused() {
		std::lock_guard<std::mutex> lock(mMutex);

		// 清理已过期的 weak_ptr
		auto it = mUUIDToResource.begin();
		while (it != mUUIDToResource.end()) {
			if (it->second.expired()) {
				// 同时清理路径映射
				for (auto pathIt = mPathToUUID.begin(); pathIt != mPathToUUID.end(); ) {
					if (pathIt->second == it->first) {
						pathIt = mPathToUUID.erase(pathIt);
					} else {
						++pathIt;
					}
				}
				it = mUUIDToResource.erase(it);
			} else {
				++it;
			}
		}
	}

	void AdResourceManager::UnloadAll() {
		std::lock_guard<std::mutex> lock(mMutex);
		mUUIDToResource.clear();
		mPathToUUID.clear();
	}

}
