#include "Resource/AdModelResource.h"
#include "Resource/AdModelLoader.h"
#include "Adlog.h"

namespace WuDu {

	AdModelResource::AdModelResource(const std::string& modelPath)
		: AdResource(modelPath),mModelPath(modelPath) {
	}

	AdModelResource::~AdModelResource() {
		Unload();
	}

	bool AdModelResource::AdModelResource::Load() {
		if (mIsLoaded) {
			return true;
		}

		LOG_I("Loading model: {0}", mModelPath);

		if (AdModelLoader::LoadModel(mModelPath, mMeshes, mMaterials)) {
			mIsLoaded = true;
			LOG_I("Model loaded successfully. Meshes: {0}, Materials: {1}", mMeshes.size(), mMaterials.size());
			return true;
		}
		else {
			LOG_E("Failed to load model: {0}", mModelPath);
			return false;
		}
	}

	void AdModelResource::Unload() {
		if (mIsLoaded) {
			mMeshes.clear();
			mMaterials.clear();
			mIsLoaded = false;
			LOG_I("Model unloaded: {0}", mModelPath);
		}
	}
}