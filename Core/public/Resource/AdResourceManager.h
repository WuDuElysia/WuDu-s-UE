#ifndef AD_RESOURCE_MANAGER_H
#define AD_RESOURCE_MANAGER_H

#include "AdResource.h"
#include "AdEngine.h"
#include "Adlog.h"

namespace WuDu {
        //资源管理器
        class AdResourceManager {
        public:
                static AdResourceManager* GetInstance();

                template<typename T, typename... Args>
                std::shared_ptr<T> Load(const std::string& path, Args&&... args) {
                        static_assert(std::is_base_of<AdResource, T>::value, "T must derive from AdResource");

                        std::lock_guard<std::mutex> lock(mMutex);

                        // 首先检查路径是否已存在
                        auto pathIt = mPathToUUID.find(path);
                        if (pathIt != mPathToUUID.end()) {
                                const UUID& uuid = pathIt->second;
                                auto uuidIt = mUUIDToResource.find(uuid);
                                if (uuidIt != mUUIDToResource.end()) {
                                        auto resource = std::dynamic_pointer_cast<T>(uuidIt->second.lock());
                                        if (resource) {
                                                return resource;
                                        }
                                }
                        }

                        // 创建新资源，资源内部会生成UUID
                        auto resource = std::make_shared<T>(path, std::forward<Args>(args)...);
                        if (resource->Load()) {
                                const UUID& uuid = resource->GetUUID();

                                // 更新双映射表
                                mUUIDToResource[uuid] = resource;
                                mPathToUUID[path] = uuid;
                        }

                        return resource;
                }

                // 通过路径获取资源
                template<typename T>
                std::shared_ptr<T> Get(const std::string& path) {
                        static_assert(std::is_base_of<AdResource, T>::value, "T must derive from AdResource");

                        std::lock_guard<std::mutex> lock(mMutex);

                        auto pathIt = mPathToUUID.find(path);
                        if (pathIt != mPathToUUID.end()) {
                                const UUID& uuid = pathIt->second;
                                return Get<T>(uuid);
                        }

                        return nullptr;
                }

                // 通过UUID获取资源
                template<typename T>
                std::shared_ptr<T> Get(const UUID& uuid) {
                        static_assert(std::is_base_of<AdResource, T>::value, "T must derive from AdResource");

                        std::lock_guard<std::mutex> lock(mMutex);

                        auto it = mUUIDToResource.find(uuid);
                        if (it != mUUIDToResource.end()) {
                                return std::dynamic_pointer_cast<T>(it->second.lock());
                        }

                        return nullptr;
                }

                // 通过UUID获取资源路径
                std::optional<std::string> GetPathByUUID(const UUID& uuid) {
                        std::lock_guard<std::mutex> lock(mMutex);

                        for (const auto& [path, resourceUUID] : mPathToUUID) {
                                if (resourceUUID == uuid) {
                                        return path;
                                }
                        }

                        return std::nullopt;
                }

                // 更新资源路径映射
                bool UpdateResourcePath(const UUID& uuid, const std::string& newPath) {
                        std::lock_guard<std::mutex> lock(mMutex);

                        // 移除旧路径映射
                        auto oldPathIt = mPathToUUID.begin();
                        while (oldPathIt != mPathToUUID.end()) {
                                if (oldPathIt->second == uuid) {
                                        oldPathIt = mPathToUUID.erase(oldPathIt);
                                        break;
                                }
                                else {
                                        ++oldPathIt;
                                }
                        }

                        // 添加新路径映射
                        mPathToUUID[newPath] = uuid;

                        // 更新资源内部路径
                        auto resourceIt = mUUIDToResource.find(uuid);
                        if (resourceIt != mUUIDToResource.end()) {
                                auto resource = resourceIt->second.lock();
                                if (resource) {
                                        // 注意：这里需要AdResource类提供SetPath方法
                                        // resource->SetPath(newPath);
                                        return true;
                                }
                        }

                        return false;
                }

                void UnloadUnused();
                void UnloadAll();

                void SetResourceRootPath(const std::string& rootPath) {
                        mRootPath = rootPath;
                }

                std::string GetFullPath(const std::string& relativePath) const {
                        return mRootPath + relativePath;
                }

        private:
                AdResourceManager();
                ~AdResourceManager();

                std::string mRootPath;

                // 双映射表实现双标识系统
                std::unordered_map<UUID, std::weak_ptr<AdResource>> mUUIDToResource;  // UUID到资源的映射
                std::unordered_map<std::string, UUID> mPathToUUID;  // 路径到UUID的映射

                std::mutex mMutex;
        };

}

#endif