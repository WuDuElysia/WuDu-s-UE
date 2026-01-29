#pragma once
#include "ECS/System/AdMaterialSystem.h"
#include "ECS/Component/Material/AdPBRMaterialComponent.h"
#include "ECS/Component/Light/AdLightComponent.h"

namespace WuDu {
#define NUM_MATERIAL_BATCH              16
#define NUM_MATERIAL_BATCH_MAX          2048
#define MAX_LIGHTS                      16

    struct PostProcessUbo {
        
    };

    class AdVKPipelineLayout;
	class AdVKPipeline;
	class AdVKDescriptorSetLayout;
	class AdVKDescriptorPool;

    class AdPBRDeferredMaterialSystem : public AdMaterialSystem {
    public:
        void OnInit(AdVKRenderPass* renderPass) override;
        void OnRender(VkCommandBuffer cmdbuffer, AdRenderTarget* renderTarget) override;
        void OnDestroy() override;
    private:
        void ReCreateMaterialDescPool(uint32_t materialCount);
        void UpdateFrameUboDescSet(AdRenderTarget* renderTarget);
        void UpdateMaterialParamsDescSet(VkDescriptorSet descSet, AdPBRMaterial* material);
        void UpdateMaterialResourceDescSet(VkDescriptorSet descSet, AdPBRMaterial* material);
        void UpdateLightUboDescSet();
        void UpdateGBufferDescSet();
        void UpdateIBLResourceDescSet();
        void UpdateLightingDescSet();
        void UpdatePostProcessDescSet();

        std::shared_ptr<AdVKDescriptorSetLayout> mFrameUboDescSetLayout;
        std::shared_ptr<AdVKDescriptorSetLayout> mLightUboDescSetLayout;
        std::shared_ptr<AdVKDescriptorSetLayout> mMaterialParamDescSetLayout;
        std::shared_ptr<AdVKDescriptorSetLayout> mMaterialResourceDescSetLayout;
        std::shared_ptr<AdVKDescriptorSetLayout> mGBufferDescSetLayout;
        std::shared_ptr<AdVKDescriptorSetLayout> mIBLResourceDescSetLayout;
        std::shared_ptr<AdVKDescriptorSetLayout> mLightingDescSetLayout;
        std::shared_ptr<AdVKDescriptorSetLayout> mPostProcessDescSetLayout;

        //GBuffer
        std::shared_ptr<AdVKPipelineLayout> mGBufferPipelineLayout;
        std::shared_ptr<AdVKPipeline> mGBufferPipeline;

        //Direct Lighting
        std::shared_ptr<AdVKPipelineLayout> mDirectLightingPipelineLayout;
        std::shared_ptr<AdVKPipeline> mDirectLightingPipeline;

        //IBL
        std::shared_ptr<AdVKPipelineLayout> mIBLPipelineLayout;
        std::shared_ptr<AdVKPipeline> mIBLPipeline;

        //Merge
        std::shared_ptr<AdVKPipelineLayout> mMergePipelineLayout;
        std::shared_ptr<AdVKPipeline> mMergePipeline;

        //PostProcess
        std::shared_ptr<AdVKPipelineLayout> mPostProcessPipelineLayout;
        std::shared_ptr<AdVKPipeline> mPostProcessPipeline;

        std::shared_ptr<AdVKDescriptorPool> mDescriptorPool;
		std::shared_ptr<AdVKDescriptorPool> mMaterialDescriptorPool;

        VkDescriptorSet mFrameUboDescSet;
        VkDescriptorSet mLightUboDescSet;
        VkDescriptorSet mGBufferDescSet;
        VkDescriptorSet mIBLResourceDescSet;
        VkDescriptorSet mLightingDescSet;
        VkDescriptorSet mPostProcessDescSet;
        std::vector<VkDescriptorSet> mMaterialDescSets;
        std::vector<VkDescriptorSet> mMaterialResourceDescSets;

        std::shared_ptr<AdVKBuffer> mFrameUbobuffer;
        std::shared_ptr<AdVKBuffer> mLightUboBuffer;
        std::shared_ptr<AdVKBuffer> mPostProcessBuffer;
        std::vector<std::shared_ptr<AdVKBuffer>> mMaterialBuffers;

        uint32_t mLastDescriptorSetCount = 0;
        std::shared_ptr<AdTexture> mDefaultTexture;
        std::shared_ptr<AdSampler> mDefaultSampler;

        //环境光参数
        glm::vec3 mAmbientColor{ 0.1f, 0.1f, 0.1f };
        float mAmbientIntensity = { 1.0f };
    };
}