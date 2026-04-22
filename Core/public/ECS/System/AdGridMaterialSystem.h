#pragma once
#include "ECS/System/AdMaterialSystem.h"

namespace WuDu {
    class AdVKPipelineLayout;
    class AdVKPipeline;
    class AdVKDescriptorSetLayout;
    class AdVKDescriptorPool;
    class AdVKBuffer;

    class AdGridMaterialSystem : public AdMaterialSystem {
    public:
        void OnInit(AdVKRenderPass* renderPass) override;
        void OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) override;
        void OnDestroy() override;

    private:
        std::shared_ptr<AdVKPipelineLayout> mPipelineLayout;
        std::shared_ptr<AdVKPipeline> mPipeline;
        std::shared_ptr<AdVKDescriptorSetLayout> mFrameUboDescSetLayout;
        std::shared_ptr<AdVKDescriptorPool> mDescriptorPool;
        VkDescriptorSet mFrameUboDescSet;
        std::shared_ptr<AdVKBuffer> mFrameUboBuffer;

        void UpdateFrameUboDescSet(AdRenderTarget* renderTarget);
    };
}
