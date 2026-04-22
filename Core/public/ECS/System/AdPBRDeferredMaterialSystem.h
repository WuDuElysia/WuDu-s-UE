#pragma once
#include "ECS/System/AdMaterialSystem.h"
#include "ECS/Component/Material/AdPBRMaterialComponent.h"
#include "ECS/Component/Light/AdLightComponent.h"

namespace WuDu {
#define NUM_MATERIAL_BATCH              16
#define NUM_MATERIAL_BATCH_MAX          2048
#define MAX_LIGHTS                      16

    // 延迟渲染专用帧 UBO（std140 布局，总计 288 字节）
    struct DeferredFrameUbo {
        glm::mat4 projMat{ 1.f };       // offset 0,   size 64
        glm::mat4 viewMat{ 1.f };       // offset 64,  size 64
        glm::mat4 invProjMat{ 1.f };    // offset 128, size 64
        glm::mat4 invViewMat{ 1.f };    // offset 192, size 64
        glm::vec3 camPos{ 0.f };        // offset 256, size 12
        alignas(4) float _pad0{ 0.f };  // offset 268, size 4
        alignas(8) glm::ivec2 resolution; // offset 272, size 8
        alignas(4) uint32_t frameId;    // offset 280, size 4
        alignas(4) float time;          // offset 284, size 4
    };
    static_assert(sizeof(DeferredFrameUbo) == 288, "DeferredFrameUbo must be 288 bytes for std140 layout");

    // 后处理 UBO（std140 布局）
    struct PostProcessUbo {
        float exposure = 1.0f;
        float bloomThreshold = 1.0f;
        float bloomIntensity = 0.04f;
        float ssaoRadius = 0.5f;
        float ssaoBias = 0.025f;
        int   ssaoKernelSize = 64;
        int   enableSSAO = 1;
        int   enableBloom = 1;
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

        void LoadIBLResources(const std::string& hdrPath);
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

        std::shared_ptr<AdVKBuffer> mFrameUboBuffer;
        std::shared_ptr<AdVKBuffer> mLightUboBuffer;
        std::shared_ptr<AdVKBuffer> mPostProcessBuffer;
        std::vector<std::shared_ptr<AdVKBuffer>> mMaterialBuffers;

        uint32_t mLastDescriptorSetCount = 0;
        std::shared_ptr<AdTexture> mDefaultTexture;
        std::shared_ptr<AdSampler> mDefaultSampler;

        // IBL 资源
        std::shared_ptr<AdTexture> mIrradianceMap;
        std::shared_ptr<AdTexture> mPrefilterMap;
        std::shared_ptr<AdTexture> mBrdfLUT;
        bool mIBLLoaded = false;

        //环境光参数
        glm::vec3 mAmbientColor{ 0.1f, 0.1f, 0.1f };
        float mAmbientIntensity = { 1.0f };

        // 当前帧的渲染目标引用（在OnRender中设置，供无参数的描述符集更新方法使用）
        AdRenderTarget* mCurrentRenderTarget = nullptr;
    };
}