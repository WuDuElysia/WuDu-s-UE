#include "AdEntryPoint.h"
#include "AdFileUtil.h"
#include "Render/AdRenderTarget.h"
#include "Render/AdMesh.h"
#include "Render/AdRenderer.h"
#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKCommandBuffer.h"

#include "ECS/AdEntity.h"
#include "ECS/System/AdBaseMaterialSystem.h"
#include "ECS/System/AdUnlitMaterialSystem.h"
#include "ECS/System/AdPBRDeferredMaterialSystem.h"
#include "ECS/System/AdSkyboxMaterialSystem.h"
#include "ECS/System/AdGridMaterialSystem.h"
#include "ECS/System/AdCameraControllerManager.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "ECS/Component/Light/AdDirectionalLightComponent.h"
#include "ECS/Component/Light/AdPointLightComponent.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"
#include "Event/AdEventAdaper.h"
#include "Gui/AdGuiSystem.h"
#include "AdTimeStep.h"
#include "AdLog.h"
#include "Resource/AdModelResource.h"
#include "Serialization/AdSceneSerializer.h"
#include "Serialization/AdBuiltinComponentSerializers.h"


/**
 * @brief PBRDeferredApp 类继承自 WuDu::AdApplication，用于演示 PBR 延迟渲染管线。
 *
 * 此类实现了应用程序生命周期的各个阶段，包括配置、初始化、场景构建、渲染和销毁。
 * 主要功能包括：
 * - 配置 8 个附件 + 5 个 Subpass 的延迟渲染通道
 * - 使用 AdPBRDeferredMaterialSystem 进行 PBR 延迟渲染
 * - 加载 3D 模型并设置 PBR 材质
 * - 创建方向光和点光源实体
 * - 创建相机实体并配置控制器
 */
class PBRDeferredApp : public WuDu::AdApplication {
protected:
	/**
	 * @brief 配置应用程序的基本设置。
	 */
	void OnConfiguration(WuDu::AppSettings* appSettings) override {
		appSettings->width = 1360;
		appSettings->height = 768;
		appSettings->title = "05_PBR_Deferred";
	}

	/**
	 * @brief 应用程序初始化阶段，创建渲染相关资源。
	 *
	 * 配置 8 个附件 + 5 个 Subpass 的延迟渲染通道：
	 * - 附件 0: GBuffer 0 - 基础颜色 (R8G8B8A8_SRGB)
	 * - 附件 1: GBuffer 1 - 法线+自发光 (R16G16B16A16_SFLOAT)
	 * - 附件 2: GBuffer 2 - 金属度+粗糙度+AO (R8G8B8A8_UNORM)
	 * - 附件 3: 深度缓冲 (D32_SFLOAT)
	 * - 附件 4: 直接光照累积 (R16G16B16A16_SFLOAT)
	 * - 附件 5: IBL 累积 (R16G16B16A16_SFLOAT)
	 * - 附件 6: 合并光照 (R16G16B16A16_SFLOAT)
	 * - 附件 7: 最终输出 (交换链表面格式)
	 */
	void OnInit() override {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();
		WuDu::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		VkFormat surfaceFormat = swapchain->GetSurfaceInfo().surfaceFormat.format;

		// ============================================================
		// 定义 8 个渲染附件
		// ============================================================
		std::vector<WuDu::Attachment> attachments = {
			// 附件 0: GBuffer 0 - 基础颜色 + Alpha
			{
				.format = VK_FORMAT_R8G8B8A8_SRGB,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			},
			// 附件 1: GBuffer 1 - 法线 + 自发光强度
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			},
			// 附件 2: GBuffer 2 - 金属度 + 粗糙度 + AO
			{
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			},
			// 附件 3: 深度缓冲
			{
				.format = VK_FORMAT_D32_SFLOAT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			},
			// 附件 4: 直接光照累积
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			},
			// 附件 5: IBL 累积
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			},
			// 附件 6: 合并光照
			{
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			},
			// 附件 7: 最终输出（交换链表面格式）
			{
				.format = surfaceFormat,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			}
		};

		// ============================================================
		// 定义 5 个 Subpass
		// ============================================================
		std::vector<WuDu::RenderSubPass> subpasses = {
			// Subpass 0: 几何阶段 - 写入 GBuffer
			{
				.inputAttachments = {},
				.colorAttachments = { 0, 1, 2 },
				.depthStencilAttachments = { 3 }
			},
			// Subpass 1: 直接光照阶段 - 读取 GBuffer，输出直接光照
			{
				.inputAttachments = { 0, 1, 2, 3 },
				.colorAttachments = { 4 },
				.depthStencilAttachments = {}
			},
			// Subpass 2: IBL 阶段 - 读取 GBuffer，输出 IBL 光照
			{
				.inputAttachments = { 0, 1, 2, 3 },
				.colorAttachments = { 5 },
				.depthStencilAttachments = {}
			},
			// Subpass 3: 合并阶段 - 读取 GBuffer + 光照结果，输出合并 HDR
			{
				.inputAttachments = { 0, 1, 4, 5 },
				.colorAttachments = { 6 },
				.depthStencilAttachments = {}
			},
			// Subpass 4: 后处理阶段 - 读取合并结果，输出最终图像
			{
				.inputAttachments = { 6 },
				.colorAttachments = { 7 },
				.depthStencilAttachments = { 3 }
			}
		};

		// 创建渲染通道（subpass 依赖关系由 AdVKRenderPass 自动生成）
		mRenderPass = std::make_shared<WuDu::AdVKRenderPass>(device, attachments, subpasses);

		// 创建渲染目标并设置清除值
		mRenderTarget = std::make_shared<WuDu::AdRenderTarget>(mRenderPass.get());
		mRenderTarget->SetColorClearValue({ 0.0f, 0.0f, 0.0f, 1.f });
		mRenderTarget->SetDepthStencilClearValue({ 1.f, 0 });

		// 添加延迟渲染材质系统
		mRenderTarget->AddMaterialSystem<WuDu::AdPBRDeferredMaterialSystem>();

		// 添加天空盒材质系统（subpass 4 后处理阶段，在 Grid 之前，利用深度缓冲被场景几何体遮挡）
		mRenderTarget->AddMaterialSystem<WuDu::AdSkyboxMaterialSystem>();

		// 添加网格材质系统（在 PBR 延迟渲染之后，保证渲染顺序正确）
		mRenderTarget->AddMaterialSystem<WuDu::AdGridMaterialSystem>();

		// 创建渲染器
		mRenderer = std::make_shared<WuDu::AdRenderer>();

		// 分配命令缓冲区
		mCmdBuffers = device->GetDefaultCmdPool()->AllocateCommandBuffer(swapchain->GetImages().size());

		// 创建立方体网格数据（用于编辑器 fallback）
		std::vector<WuDu::AdVertex> vertices;
		std::vector<uint32_t> indices;
		WuDu::AdGeometryUtil::CreateCube(-0.3f, 0.3f, -0.3f, 0.3f, -0.3f, 0.3f, vertices, indices);
		mCubeMesh = std::make_shared<WuDu::AdMesh>(vertices, indices);

		// 创建编辑器 fallback 材质（用于 GUI 系统）
		mEditorFallbackMaterial = std::shared_ptr<WuDu::AdPBRMaterial>(
			WuDu::AdMaterialFactory::GetInstance()->CreateMaterial<WuDu::AdPBRMaterial>());
		mEditorFallbackMaterial->SetMetallicFactor(0.0f);
		mEditorFallbackMaterial->SetRoughnessFactor(0.5f);

		// 初始化 GUI 系统
		mGuiSystem = std::make_shared<WuDu::AdGuiSystem>();
		mGuiSystem->OnInit();

		// 注册内置组件序列化器
		WuDu::AdBuiltinComponentSerializers::RegisterAll();
	}

	/**
	 * @brief 初始化场景内容，创建摄像机、模型实体和光源。
	 */
	void OnSceneInit(WuDu::AdScene* scene) override {
		// 从场景文件加载场景内容
		uint32_t entityCount = 0;
		uint32_t resourceCount = 0;
		std::string sceneFilePath = std::string(AD_RES_ROOT_DIR) + "Scenes/default.scene.json";

		if (!WuDu::AdSceneSerializer::Load(scene, sceneFilePath, &entityCount, &resourceCount)) {
			LOG_W("Failed to load default scene file: {0}. Starting with empty scene.", sceneFilePath);
		} else {
			LOG_I("Loaded scene: {0} entities, {1} resources", entityCount, resourceCount);
		}

		// 查找或创建摄像机实体
		WuDu::AdEntity* camera = nullptr;
		auto& ecsRegistry = scene->GetEcsRegistry();
		auto cameraView = ecsRegistry.view<WuDu::AdFirstPersonCameraComponent>();
		for (auto enttEntity : cameraView) {
			camera = scene->GetEntity(enttEntity);
			if (camera) break;
		}
		if (!camera) {
			camera = scene->CreateEntity("Editor Camera");
			camera->AddComponent<WuDu::AdFirstPersonCameraComponent>();
		}

		m_CameraController = std::make_unique<WuDu::AdCameraControllerManager>(camera);
		m_CameraController->SetAspect(1360.0f / 768.0f);
		mRenderTarget->SetCamera(camera);

		// 设置 GUI 系统所需资源（使用 fallback cube mesh 和材质）
		mGuiSystem->SetResources(
			mScene.get(),
			camera,
			mCubeMesh.get(),
			mEditorFallbackMaterial.get()
		);
		mGuiSystem->AddSceneEditor();

		// 加载 IBL 资源（当前为 stub，仅记录日志）
		auto* deferredSystem = mRenderTarget->GetMaterialSystem<WuDu::AdPBRDeferredMaterialSystem>();
		if (deferredSystem) {
			deferredSystem->LoadIBLResources("Resource/HDR/environment.hdr");
		}
	}

	void OnUpdate(float deltaTime) override {
		WuDu::InputManager::GetInstance().ProcessEvents();

		if (m_CameraController) {
			m_CameraController->Update(deltaTime);
		}
	}

	void OnSceneDestroy(WuDu::AdScene* scene) override {
	}

	/**
	 * @brief 每帧渲染回调函数。
	 */
	void OnRender() override {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		// 在录制命令缓冲区之前处理延迟纹理加载，确保新纹理在录制时已就位，
		// 避免录制后销毁旧纹理导致命令缓冲区引用已销毁的 VkImageView
		mGuiSystem->ProcessPendingTextureLoads();

		int32_t imageIndex;
		if (mRenderer->Begin(&imageIndex)) {
			mRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
			mGuiSystem->RebuildResources();
		}

		VkCommandBuffer cmdBuffer = mCmdBuffers[imageIndex];
		WuDu::AdVKCommandPool::BeginCommandBuffer(cmdBuffer);

		// 渲染 3D 场景
		mRenderTarget->Begin(cmdBuffer);
		mRenderTarget->RenderMaterialSystems(cmdBuffer);
		mRenderTarget->End(cmdBuffer);

		WuDu::AdVKCommandPool::EndCommandBuffer(cmdBuffer);

		// 录制 GUI 命令（不做独立的 acquire/present）
		mGuiSystem->BeginGui();
		mGuiSystem->EndGui();
		VkCommandBuffer guiCmdBuffer = mGuiSystem->OnRenderGui(imageIndex);

		// 场景 + GUI 命令缓冲区一起提交，只做一次 present
		std::vector<VkCommandBuffer> allCmds = { cmdBuffer };
		if (guiCmdBuffer != VK_NULL_HANDLE) {
			allCmds.push_back(guiCmdBuffer);
		}
		if (mRenderer->End(imageIndex, allCmds)) {
			mRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
		}
	}

	/**
	 * @brief 应用程序销毁阶段，释放所有已创建的资源。
	 */
	void OnDestroy() override {
		WuDu::AdRenderContext* renderCxt = WuDu::AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();
		vkDeviceWaitIdle(device->GetHandle());

		mGuiSystem->OnDestroy();
		mGuiSystem.reset();

		mEditorFallbackMaterial.reset();
		mCubeMesh.reset();
		mCmdBuffers.clear();
		mRenderTarget.reset();
		mRenderPass.reset();
		mRenderer.reset();
	}

private:
	std::shared_ptr<WuDu::AdVKRenderPass> mRenderPass;
	std::shared_ptr<WuDu::AdRenderTarget> mRenderTarget;
	std::shared_ptr<WuDu::AdRenderer> mRenderer;

	std::vector<VkCommandBuffer> mCmdBuffers;
	std::shared_ptr<WuDu::AdMesh> mCubeMesh;

	std::unique_ptr<WuDu::AdCameraControllerManager> m_CameraController;

	std::shared_ptr<WuDu::AdPBRMaterial> mEditorFallbackMaterial;
	std::shared_ptr<WuDu::AdGuiSystem> mGuiSystem;
};

/**
 * @brief 入口点函数，用于创建应用程序实例。
 */
WuDu::AdApplication* CreateApplicationEntryPoint() {
	return new PBRDeferredApp();
}
