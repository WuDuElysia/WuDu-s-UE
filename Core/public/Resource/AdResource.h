
#ifndef AD_RESOURCE_H
#define AD_RESOURCE_H

#include "AdEngine.h"

namespace WuDu {

        //添加状态
        enum class ResourceState {
                Unloaded,
                Loading,
                Loaded,
                Failed
        };
        //资源基础类
        class AdResource {
        public:
                AdResource(const std::string& resourcePath);
                virtual ~AdResource();

                virtual bool Load() = 0;
                virtual void Unload() = 0;
                virtual bool Reload();

                const std::string& GetPath() const { return mPath; }
                ResourceState GetState() const { return mState; }
                bool IsLoaded() const { return mState == ResourceState::Loaded; }

                void AddReference() { mRefCount.fetch_add(1); }
                void RemoveReference() {
                        if (mRefCount.fetch_sub(1) == 1) {
                                Unload();
                        }
                }

        protected:
                std::string mPath;
                ResourceState mState;
                std::atomic<int32_t> mRefCount;
        };

} 

#endif 