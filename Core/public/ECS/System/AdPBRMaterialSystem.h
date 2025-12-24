#pragma once

#include "ECS/System/AdMaterialSystem.h"
#include "ECS/Component/Material/AdPBRMaterialComponent.h"

namespace WuDu {
#define NUM_MATERIAL_BATCH              16
#define NUM_MATERIAL_BATCH_MAX          2048

        class AdVKPipelineLayout;
        class AdVKPipeline;
        class AdVKDescriptorSetLayout;
        class AdVKDescriptorPool;

        class AdPBRMaterialSystem : public AdMaterialSystem {
        public:
                void OnInit(AdVKRenderPass* renderPass) override;
                void OnRender(VkCommandBuffer cmdbuffer, AdRenderTarget* renderTarget) override;
                void OnDestroy() override;
        private:
                void ReCreateMaterialDescPool(uint32_t materialCount);
                void UpdateFrameUboDescSet(AdRenderTarget* renderTarget);
                void UpdateMaterialParamsDescSet(VkDescriptorSet descSet, AdPBRMaterial* metarial);
                void UpdateMaterialResourceDescSet(VkDescriptorSet descSet, AdPBRMaterial* material);

                std::shared_ptr<AdVKDescriptorSetLayout> mFrameUboDescSetLayout;
                std::shared_ptr<AdVKDescriptorSetLayout> mMaterialParamDescSetLayout;
                std::shared_ptr<AdVKDescriptorSetLayout> mMaterialResourceDescSetLayout;

                std::shared_ptr<AdVKPipelineLayout> mPipelineLayout;
                std::shared_ptr<AdVKPipeline> mPipeline;

                std::shared_ptr<AdVKDescriptorPool> mDescriptorPool;
                std::shared_ptr<AdVKDescriptorPool> mMaterialDescriptorPool;

                VkDescriptorSet mFrameUboDescSet;
                std::shared_ptr<AdVKBuffer> mFrameUboBuffer;

                uint32_t mLastDescriptorSetCount = 0;
                std::vector<VkDescriptorSet> mMaterialDescSets;
                std::vector<VkDescriptorSet> mMaterialResourceDescSets;
                std::vector<std::shared_ptr<AdVKBuffer>> mMaterialBuffers;
                std::shared_ptr<AdTexture> mDefaultTexture;
                std::shared_ptr<AdSampler> mDefaultSampler;

        };
}