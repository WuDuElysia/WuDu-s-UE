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
#include "ECS/System/AdCameraControllerManager.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"
#include "Event/AdEventAdaper.h"
#include "Gui/AdGuiSystem.h"
#include "AdTimeStep.h"
#include "AdLog.h"
#include "Resource/AdModelResource.h"


/**
 * @brief SandBoxApp 类继承自 WuDu::AdApplication，用于演示基于 ECS 的实体渲染示例。
 *
 * 此类实现了应用程序生命周期的各个阶段，包括配置、初始化、场景构建、渲染和销毁。
 * 主要功能包括：
 * - 设置窗口大小和标题
 * - 初始化渲染资源（渲染通道、目标、命令缓冲区等）
 * - 构建包含多个立方体实体的场景
 * - 实现每帧渲染逻辑
 * - 清理资源
 */
class SandBoxApp : public WuDu::AdApplication {
protected:
	/**
	 * @brief 配置应用程序的基本设置。
	 *
	 * @param appSettings 指向 AppSettings 对象的指针，用于设置窗口宽度、高度和标题。
	 */
	void OnConfiguration(WuDu::AppSettings* appSettings) override {
		appSettings->width = 1360;
		appSettings->height = 768;
		appSettings->title = "04_ECS_Entity";
	}

	/**
	 * @brief 应用程序初始化阶段，创建渲染相关资源。
	 *
	 * 包括：
	 * - 创建渲染通道（颜色附件和深度附件）
	 * - 创建渲染目标并设置清除值
	 * - 创建渲染器和命令缓冲区
	 * - 构建立方体网格数据
	 */
	void OnInit() override {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();
		WuDu::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		// 定义渲染附件：颜色附件和深度附件
		std::vector<WuDu::Attachment> attachments = {
		    {
			.format = swapchain->GetSurfaceInfo().surfaceFormat.format,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
		    },
		    {
			.format = device->GetSettings().depthFormat,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
		    }
		};

		// 定义子通道：使用颜色附件和深度附件
		std::vector<WuDu::RenderSubPass> subpasses = {
		    {
			.colorAttachments = { 0 },
			.depthStencilAttachments = { 1 },
			.sampleCount = VK_SAMPLE_COUNT_4_BIT
		    }
		};


		// 创建渲染通道
		mRenderPass = std::make_shared<WuDu::AdVKRenderPass>(device, attachments, subpasses);
		// 创建渲染目标并设置清除值
		mRenderTarget = std::make_shared<WuDu::AdRenderTarget>(mRenderPass.get());
		mRenderTarget->SetColorClearValue({ 0.1f, 0.2f, 0.3f, 1.f });
		mRenderTarget->SetDepthStencilClearValue({ 1, 0 });
		mRenderTarget->AddMaterialSystem<WuDu::AdUnlitMaterialSystem>();
		mRenderTarget->AddMaterialSystem<WuDu::AdBaseMaterialSystem>();


		// 创建渲染器
		mRenderer = std::make_shared<WuDu::AdRenderer>();

		// 分配命令缓冲区
		mCmdBuffers = device->GetDefaultCmdPool()->AllocateCommandBuffer(swapchain->GetImages().size());


		// 创建立方体网格数据
		std::vector<WuDu::AdVertex> vertices;
		std::vector<uint32_t> indices;
		WuDu::AdGeometryUtil::CreateCube(-0.3f, 0.3f, -0.3f, 0.3f, -0.3f, 0.3f, vertices, indices);
		mCubeMesh = std::make_shared<WuDu::AdMesh>(vertices, indices);

		//加载模型
		std::shared_ptr<WuDu::AdModelResource> model = std::make_shared<WuDu::AdModelResource>(AD_RES_MODEL_DIR"Phainon.fbx");
		std::vector<WuDu::ModelVertex> Vertices;
		std::vector<uint32_t> Indices;
		if (model->Load()) {
			const std::vector<WuDu::ModelMesh>& meshes = model->GetMeshes();

			//处理每个网格
			/*for (const auto& mesh : meshes) {
				
			}*/
			Vertices = meshes[0].Vertices;
			Indices = meshes[0].Indices;
			mModelMeshes.emplace_back(std::make_shared<WuDu::AdMesh>(Vertices, Indices));
		}
		else {
			mModelMeshes.emplace_back(std::make_shared<WuDu::AdMesh>(vertices, indices));
		}
		//mModelMesh = std::make_shared<WuDu::AdMesh>(Vertices, Indices);
		//mModelMeshes.emplace_back(std::make_shared<WuDu::AdMesh>(Vertices,Indices));

		// 创建材质
		mBaseMaterial = std::shared_ptr<WuDu::AdUnlitMaterial>(WuDu::AdMaterialFactory::GetInstance()->CreateMaterial<WuDu::AdUnlitMaterial>());
		mTexture0 = std::make_shared<WuDu::AdTexture>(AD_RES_TEXTURE_DIR"R-C.jpeg");

		WuDu::AdSampler::Settings samplerSettings{};
		samplerSettings.minFilter = VK_FILTER_LINEAR;
		samplerSettings.magFilter = VK_FILTER_LINEAR;
		samplerSettings.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerSettings.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		mSampler = std::make_shared<WuDu::AdSampler>(samplerSettings);

		mBaseMaterial->SetTextureView(0, mTexture0.get(), mSampler.get());
		mBaseMaterial->SetBaseColor0(glm::vec3(0.5f, 0.5f, 0.5f));


		// 1. 初始化GUI系统
		mGuiSystem = std::make_shared<WuDu::AdGuiSystem>();
		mGuiSystem->OnInit();





		// 4. 添加自定义GUI函数（可选）



	}

	/**
	 * @brief 初始化场景内容，创建摄像机和多个立方体实体。
	 *
	 * @param scene 指向当前场景对象的指针。
	 */
	void OnSceneInit(WuDu::AdScene* scene) override {

		// 创建摄像机实体并设置其组件
		WuDu::AdEntity* camera = mScene->CreateEntity("Editor Camera");
		auto& cameraComp = camera->AddComponent<WuDu::AdFirstPersonCameraComponent>();
		// 初始化相机控制器管理器
		m_CameraController = std::make_unique<WuDu::AdCameraControllerManager>(camera);
		m_CameraController->SetAspect(1360.0f / 768.0f);

		mRenderTarget->SetCamera(camera);

		// 2. 设置GUI系统所需资源
		mGuiSystem->SetResources(
			mScene.get(),                  // 场景
			camera,                  // 活动摄像机
			mCubeMesh.get(),         // 立方体网格
			mBaseMaterial.get()      // 默认材质
		);

		// 3. 添加场景编辑器功能
		mGuiSystem->AddSceneEditor();


		// 创建多个实体，并设置其材质和变换属性
		{
			mCubes.emplace_back(scene->CreateEntity("MiG-29"));
			auto& materialComp = mCubes[0]->AddComponent<WuDu::AdUnlitMaterialComponent>();
			materialComp.AddMesh(mModelMeshes[0].get(), mBaseMaterial.get());
			auto& transComp = mCubes[0]->GetComponent<WuDu::AdTransformComponent>();
			transComp.scale = { 0.4f, 0.4f, 0.4f };
			transComp.position = { 0.f, 0.f, 0.0f };
			transComp.rotation = { 0.f, 0.f, 0.f };
		}
	}

	void OnUpdate(float deltaTime) override {

		//处理队列事件
		WuDu::InputManager::GetInstance().ProcessEvents();

		// 更新相机控制器
		if (m_CameraController) {
			m_CameraController->Update(deltaTime);
		}

		// 旋转速度（度/秒）
		float rotationSpeed = 90.0f; // 每秒旋转90度

		// 更新 Cube 0 - 绕Y轴旋转
		if (mCubes[0] && mCubes[0]->HasComponent<WuDu::AdTransformComponent>()) {
			auto& transComp = mCubes[0]->GetComponent<WuDu::AdTransformComponent>();
			transComp.rotation.y += rotationSpeed * deltaTime;
			// 保持在0-360度范围内
			if (transComp.rotation.y >= 360.0f) {
				transComp.rotation.y -= 360.0f;
			}
		}

	}

	/**
	 * @brief 场景销毁回调函数，当前为空实现。
	 *
	 * @param scene 指向即将销毁的场景对象的指针。
	 */
	void OnSceneDestroy(WuDu::AdScene* scene) override {

		mTexture0.reset();
		mTexture1.reset();
		mSampler.reset();
	}

	/**
	 * @brief 每帧渲染回调函数，执行渲染流程。
	 *
	 * 包括：
	 * - 获取交换链图像索引
	 * - 开始记录命令缓冲区
	 * - 执行渲染目标的渲染操作
	 * - 提交命令缓冲区并呈现图像
	 */
	void OnRender() override {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		int32_t imageIndex;
		if (mRenderer->Begin(&imageIndex)) {
			mRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
			mGuiSystem->RebuildResources();
		}

		VkCommandBuffer cmdBuffer = mCmdBuffers[imageIndex];
		WuDu::AdVKCommandPool::BeginCommandBuffer(cmdBuffer);

		// 渲染3D场景
		mRenderTarget->Begin(cmdBuffer);
		mRenderTarget->RenderMaterialSystems(cmdBuffer);
		mRenderTarget->End(cmdBuffer);


		WuDu::AdVKCommandPool::EndCommandBuffer(cmdBuffer);
		if (mRenderer->End(imageIndex, { cmdBuffer })) {
			mRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
		}
		mGuiSystem->BeginGui();      // 开始GUI帧（内部会调用UI构建函数）
		// 3. 结束GUI帧
		mGuiSystem->EndGui();
		// 4. 渲染GUI
		mGuiSystem->OnRender();

	}


	/**
	 * @brief 应用程序销毁阶段，释放所有已创建的资源。
	 */
	void OnDestroy() override {
		WuDu::AdRenderContext* renderCxt = WuDu::AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();
		vkDeviceWaitIdle(device->GetHandle());

		// 关键：先清理GUI系统

		mGuiSystem->OnDestroy();
		mGuiSystem.reset();


		mCubeMesh.reset();
		mCmdBuffers.clear();
		mRenderTarget.reset();
		mRenderPass.reset();
		mRenderer.reset();

	}

private:
	std::shared_ptr<WuDu::AdVKRenderPass> mRenderPass;       ///< 渲染通道对象
	std::shared_ptr<WuDu::AdRenderTarget> mRenderTarget;     ///< 渲染目标对象
	std::shared_ptr<WuDu::AdRenderer> mRenderer;             ///< 渲染器对象

	std::vector<VkCommandBuffer> mCmdBuffers;               ///< 命令缓冲区数组
	std::shared_ptr<WuDu::AdMesh> mCubeMesh;                 ///< 立方体网格对象
	std::shared_ptr<WuDu::AdMesh> mModelMesh;
	std::vector<std::shared_ptr<WuDu::AdMesh>> mModelMeshes;		//模型网格体组
	std::vector<WuDu::AdEntity*> mCubes;

	std::unique_ptr<WuDu::AdCameraControllerManager> m_CameraController;  ///< 相机控制器管理器

	//材质
	std::shared_ptr<WuDu::AdTexture> mTexture0;
	std::shared_ptr<WuDu::AdTexture> mTexture1;
	std::shared_ptr<WuDu::AdSampler> mSampler;
	std::shared_ptr<WuDu::AdUnlitMaterial> mBaseMaterial;
	std::shared_ptr<WuDu::AdGuiSystem>mGuiSystem;




	float m_LastTime = 0.0f;


};

/**
 * @brief 入口点函数，用于创建应用程序实例。
 *
 * @return 返回指向新创建的 SandBoxApp 实例的指针。
 */
WuDu::AdApplication* CreateApplicationEntryPoint() {
	return new SandBoxApp();
}