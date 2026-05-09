#pragma once
#include "ECS/System/AdMaterialSystem.h"

namespace WuDu {
    struct SkyboxUbo {
        float timeScale      = 1.0f;
        float exposure       = 1.0f;
        float starIntensity  = 1.0f;
        float auroraHeight   = 0.2f;
    };

    class AdVKPipelineLayout;
    class AdVKPipeline;
    class AdVKDescriptorSetLayout;
    class AdVKDescriptorPool;
    class AdVKBuffer;

    class AdSkyboxMaterialSystem : public AdMaterialSystem {
    public:
        void OnInit(AdVKRenderPass* renderPass) override;
        void OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) override;
        void OnDestroy() override;

        // Runtime parameter control
        void  SetTimeScale(float v)     { mSkyboxParams.timeScale = v; }
        void  SetExposure(float v)      { mSkyboxParams.exposure = v; }
        void  SetStarIntensity(float v) { mSkyboxParams.starIntensity = v; }
        void  SetAuroraHeight(float v)  { mSkyboxParams.auroraHeight = v; }
        float GetTimeScale() const      { return mSkyboxParams.timeScale; }
        float GetExposure() const       { return mSkyboxParams.exposure; }
        float GetStarIntensity() const  { return mSkyboxParams.starIntensity; }
        float GetAuroraHeight() const   { return mSkyboxParams.auroraHeight; }

    private:
        void UpdateFrameUboDescSet(AdRenderTarget* renderTarget);
        void UpdateSkyboxUboDescSet();

        std::shared_ptr<AdVKPipelineLayout>      mPipelineLayout;
        std::shared_ptr<AdVKPipeline>            mPipeline;
        std::shared_ptr<AdVKDescriptorSetLayout> mFrameUboDescSetLayout;
        std::shared_ptr<AdVKDescriptorSetLayout> mSkyboxUboDescSetLayout;
        std::shared_ptr<AdVKDescriptorPool>      mDescriptorPool;
        VkDescriptorSet                          mFrameUboDescSet;
        VkDescriptorSet                          mSkyboxUboDescSet;
        std::shared_ptr<AdVKBuffer>              mFrameUboBuffer;
        std::shared_ptr<AdVKBuffer>              mSkyboxUboBuffer;

        SkyboxUbo mSkyboxParams;  // Initialized with default values
    };
}
