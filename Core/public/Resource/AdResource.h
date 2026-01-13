
#ifndef AD_RESOURCE_H
#define AD_RESOURCE_H

#include "AdEngine.h"
#include "Adlog.h"

namespace WuDu {


	enum class ResourceState {
		Unloaded,
		Loading,
		Loaded,
		Failed
	};


	typedef std::string UUID;

	inline UUID CreateUUID() {
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<> dis(0, 15);
		static std::uniform_int_distribution<> dis2(8, 11);

		std::stringstream ss;
		int i;
		ss << std::hex;
		for (i = 0; i < 8; i++) {
			ss << dis(gen);
		}
		ss << "-";
		for (i = 0; i < 4; i++) {
			ss << dis(gen);
		}
		ss << "-4";
		for (i = 0; i < 3; i++) {
			ss << dis(gen);
		}
		ss << "-";
		ss << dis2(gen);
		for (i = 0; i < 3; i++) {
			ss << dis(gen);
		}
		ss << "-";
		for (i = 0; i < 12; i++) {
			ss << dis(gen);
		}
		return ss.str();
	}


	enum class ResourceIdType {
		path,
		UUID
	};

	class AdResource {
	public:
		AdResource(const std::string& resourcePath) :mPath(resourcePath), mUUID(CreateUUID()), mState(ResourceState::Unloaded), mRefCount(0) {}
		virtual ~AdResource() = default;

		virtual bool Load() = 0;
		virtual void Unload() = 0;
		//virtual bool Reload();

		const std::string& GetPath() const { return mPath; }
		ResourceState GetState() const { return mState; }
		bool IsLoaded() const { return mState == ResourceState::Loaded; }

		void AddReference() {
			mRefCount.fetch_add(1);
			LOG_D("Resource AddReference: {0}", mPath);
		}
		void RemoveReference() {
			if (mRefCount.fetch_sub(1) == 1) {
				Unload();
				LOG_W("Resource unload: {0}", mPath);
			}
		}

	protected:
		UUID mUUID;
		std::string mPath;
		ResourceState mState;
		std::atomic<int32_t> mRefCount;
	};

}

#endif 