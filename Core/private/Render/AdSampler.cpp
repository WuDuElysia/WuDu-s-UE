
#include "Render/AdSampler.h"
#include "AdApplication.h"

namespace WuDu {
	AdSampler::AdSampler(const Settings& settings)
	{
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		mDevice = renderCxt->GetDevice();
		CreateSampler(settings);
	}

	AdSampler::AdSampler()
	{
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		mDevice = renderCxt->GetDevice();
		Settings defaultSettings;
		CreateSampler(defaultSettings);
	}

	AdSampler::~AdSampler() {
		if (mHandle != VK_NULL_HANDLE) {
			vkDestroySampler(mDevice->GetHandle(), mHandle, nullptr);
		}
	}

	void AdSampler::CreateSampler(const Settings& settings) {
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = settings.magFilter;
		samplerInfo.minFilter = settings.minFilter;
		samplerInfo.addressModeU = settings.addressModeU;
		samplerInfo.addressModeV = settings.addressModeV;
		samplerInfo.addressModeW = settings.addressModeW;
		samplerInfo.anisotropyEnable = settings.anisotropyEnable;
		samplerInfo.maxAnisotropy = settings.maxAnisotropy;
		samplerInfo.borderColor = settings.borderColor;
		samplerInfo.unnormalizedCoordinates = settings.unnormalizedCoordinates;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

		if (vkCreateSampler(mDevice->GetHandle(), &samplerInfo, nullptr, &mHandle) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}
}