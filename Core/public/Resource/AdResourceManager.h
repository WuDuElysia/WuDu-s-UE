#ifndef AD_RESOURCE_MANAGER_H
#define AD_RESOURCE_MANAGER_H

#include "AdResource.h"
#include <unordered_map>
#include <mutex>
#include <functional>

namespace WuDu {
        //×ÊÔ´¼ÓÔØÆ÷
        class AdResourceManager {
        public:
                static AdResourceManager* GetInstance();

                template<typename T, typename... Args>
                std::shared_ptr<T> Load(const std::string& path, Args&&... args) {
                        static_assert(std::is_base_of<AdResource, T>::value, "T must derive from AdResource");

                        std::lock_guard<std::mutex> lock(mMutex);

                        auto it = mResources.find(path);
                        if (it != mResources.end()) {
                                auto resource = std::dynamic_pointer_cast<T>(it->second.lock());
                                if (resource) {
                                        return resource;
                                }
                        }

                        auto resource = std::make_shared<T>(path, std::forward<Args>(args)...);
                        if (resource->Load()) {
                                mResources[path] = resource;
                        }

                        return resource;
                }

                template<typename T>
                std::shared_ptr<T> Get(const std::string& path) {
                        static_assert(std::is_base_of<AdResource, T>::value, "T must derive from AdResource");

                        std::lock_guard<std::mutex> lock(mMutex);

                        auto it = mResources.find(path);
                        if (it != mResources.end()) {
                                return std::dynamic_pointer_cast<T>(it->second.lock());
                        }

                        return nullptr;
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
                std::unordered_map<std::string, std::weak_ptr<AdResource>> mResources;
                std::mutex mMutex;
        };

} 

#endif