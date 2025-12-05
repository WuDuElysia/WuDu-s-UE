#pragma once
#include "Graphic/AdVKCommon.h"
#include "Render/AdRenderContext.h"

namespace WuDu {
	class AdSampler {
	public:
		// 添加设置结构体
		struct Settings {
			VkFilter minFilter = VK_FILTER_LINEAR;
			VkFilter magFilter = VK_FILTER_LINEAR;
			VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			VkBool32 anisotropyEnable = VK_FALSE;
			float maxAnisotropy = 1.0f;
			VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			VkBool32 unnormalizedCoordinates = VK_FALSE;

			Settings() = default;
		};

		// 构造函数
		AdSampler(const Settings& settings);
		AdSampler(); // 默认构造函数
		~AdSampler();

		VkSampler GetHandle() const { return mHandle; }

	private:
		AdVKDevice* mDevice;
		VkSampler mHandle = VK_NULL_HANDLE;
		void CreateSampler(const Settings& settings);
	};
}