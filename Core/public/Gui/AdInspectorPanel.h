// AdInspectorPanel.h
#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace WuDu {
	struct AdEditorContext;
	struct AdComponentInfo;
	struct AdPropertyDescriptor;
	class AdTexture;
	class AdSampler;

	class AdInspectorPanel {
	public:
		// Deferred texture load request — records a drag-drop that will be
		// executed outside the ImGui render loop to avoid GPU deadlock.
		struct PendingTextureLoad {
			uint32_t slotId;
			std::string texturePath;
		};

		void OnImGui(AdEditorContext& context);

		// Process queued texture loads outside the render loop.
		void ProcessPendingTextureLoads(AdEditorContext& context);

	private:
		void DrawComponent(AdEditorContext& context,
						   const AdComponentInfo& info, void* component);
		void DrawProperty(const AdPropertyDescriptor& prop, void* component);
		void DrawPBRTextureSlots(AdEditorContext& context);
		void DrawAddComponentButton(AdEditorContext& context);

		// Texture/sampler ownership for drag-drop loaded resources
		std::unordered_map<uint32_t, std::shared_ptr<AdTexture>> mLoadedTextures;
		std::unordered_map<uint32_t, std::string> mLoadedTexturePaths;
		std::shared_ptr<AdSampler> mDefaultSampler;

		// Queue of pending texture load requests (filled during ImGui render,
		// drained between frames by ProcessPendingTextureLoads).
		std::vector<PendingTextureLoad> mPendingTextureLoads;

		// Error tracking for texture load failures
		std::string mTextureLoadError;
		float mErrorTimestamp = 0.0f;
		int mErrorSlotId = -1;
	};
}
