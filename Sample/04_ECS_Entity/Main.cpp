#include "AdEntryPoint.h"
#include "AdFileUtil.h"
#include "Render/AdRenderTarget.h"
#include "Render/AdMesh.h"
#include "Render/AdRenderer.h"
#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKCommandBuffer.h"

#include "ECS/AdEntity.h"
#include "ECS/System/AdBaseMaterialSystem.h"
#include "ECS/System/AdCameraControllerManager.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"
#include "Event/AdEventAdaper.h"
#include "AdLog.h"

/**
 * @brief SandBoxApp 类继承自 ade::AdApplication，用于演示基于 ECS 的实体渲染示例。
 *
 * 此类实现了应用程序生命周期的各个阶段，包括配置、初始化、场景构建、渲染和销毁。
 * 主要功能包括：
 * - 设置窗口大小和标题
 * - 初始化渲染资源（渲染通道、目标、命令缓冲区等）
 * - 构建包含多个立方体实体的场景
 * - 实现每帧渲染逻辑
 * - 清理资源
 */
class SandBoxApp : public ade::AdApplication {
protected:
	/**
	 * @brief 配置应用程序的基本设置。
	 *
	 * @param appSettings 指向 AppSettings 对象的指针，用于设置窗口宽度、高度和标题。
	 */
	void OnConfiguration(ade::AppSettings* appSettings) override {
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
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		// 定义渲染附件：颜色附件和深度附件
		std::vector<ade::Attachment> attachments = {
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
		std::vector<ade::RenderSubPass> subpasses = {
		    {
			.colorAttachments = { 0 },
			.depthStencilAttachments = { 1 },
			.sampleCount = VK_SAMPLE_COUNT_4_BIT
		    }
		};

		// 创建渲染通道
		mRenderPass = std::make_shared<ade::AdVKRenderPass>(device, attachments, subpasses);

		// 创建渲染目标并设置清除值
		mRenderTarget = std::make_shared<ade::AdRenderTarget>(mRenderPass.get());
		mRenderTarget->SetColorClearValue({ 0.1f, 0.2f, 0.3f, 1.f });
		mRenderTarget->SetDepthStencilClearValue({ 1, 0 });
		mRenderTarget->AddMaterialSystem<ade::AdBaseMaterialSystem>();

		// 创建渲染器
		mRenderer = std::make_shared<ade::AdRenderer>();

		// 分配命令缓冲区
		mCmdBuffers = device->GetDefaultCmdPool()->AllocateCommandBuffer(swapchain->GetImages().size());
		

		// 创建立方体网格数据
		std::vector<ade::AdVertex> vertices;
		std::vector<uint32_t> indices;
		ade::AdGeometryUtil::CreateCube(-0.3f, 0.3f, -0.3f, 0.3f, -0.3f, 0.3f, vertices, indices);
		mCubeMesh = std::make_shared<ade::AdMesh>(vertices, indices);

		//设置相机控制
		SetupCameraControls();
	}

	/**
	 * @brief 初始化场景内容，创建摄像机和多个立方体实体。
	 *
	 * @param scene 指向当前场景对象的指针。
	 */
	void OnSceneInit(ade::AdScene* scene) override {
		// 创建摄像机实体并设置其组件
		ade::AdEntity* camera = scene->CreateEntity("Editor Camera");
		auto& cameraComp = camera->AddComponent<ade::AdFirstPersonCameraComponent>();
		// 初始化相机控制器管理器
		m_CameraController = std::make_unique<ade::AdCameraControllerManager>(camera);
		m_CameraController->SetAspect(1360.0f / 768.0f);
		
		mRenderTarget->SetCamera(camera);

		// 创建两种基础材质
		auto baseMat0 = ade::AdMaterialFactory::GetInstance()->CreateMaterial<ade::AdBaseMaterial>();
		baseMat0->colorType = ade::COLOR_TYPE_NORMAL;
		auto baseMat1 = ade::AdMaterialFactory::GetInstance()->CreateMaterial<ade::AdBaseMaterial>();
		baseMat1->colorType = ade::COLOR_TYPE_TEXCOORD;

		
		// 创建多个立方体实体，并设置其材质和变换属性
		{
			mCubes.emplace_back(scene->CreateEntity("Cube 0"));
			auto& materialComp = mCubes[0]->AddComponent<ade::AdBaseMaterialComponent>();
			materialComp.AddMesh(mCubeMesh.get(), baseMat1);
			auto& transComp = mCubes[0]->GetComponent<ade::AdTransformComponent>();
			transComp.scale = { 1.f, 1.f, 1.f };
			transComp.position = { 0.f, 0.f, 0.0f };
			transComp.rotation = { 17.f, 30.f, 0.f };
		}
		{
			mCubes.emplace_back(scene->CreateEntity("Cube 1"));
			auto& materialComp = mCubes[1]->AddComponent<ade::AdBaseMaterialComponent>();
			materialComp.AddMesh(mCubeMesh.get(), baseMat0);
			auto& transComp = mCubes[1]->GetComponent<ade::AdTransformComponent>();
			transComp.scale = { 0.5f, 0.5f, 0.5f };
			transComp.position = { -1.f, 0.f, 0.0f };
			transComp.rotation = { 17.f, 30.f, 0.f };
		}
		{
			mCubes.emplace_back(scene->CreateEntity("Cube 2"));
			auto& materialComp = mCubes[2]->AddComponent<ade::AdBaseMaterialComponent>();
			materialComp.AddMesh(mCubeMesh.get(), baseMat1);
			auto& transComp = mCubes[2]->GetComponent<ade::AdTransformComponent>();
			transComp.scale = { 0.5f, 0.5f, 0.5f };
			transComp.position = { 1.f, 0.f, 0.0f };
			transComp.rotation = { 17.f, 30.f, 0.f };
		}
		{
			mCubes.emplace_back(scene->CreateEntity("Cube 3"));
			auto& materialComp = mCubes[3]->AddComponent<ade::AdBaseMaterialComponent>();
			materialComp.AddMesh(mCubeMesh.get(), baseMat1);
			auto& transComp = mCubes[3]->GetComponent<ade::AdTransformComponent>();
			transComp.scale = { 0.5f, 0.5f, 0.5f };
			transComp.position = { 0.f, 1.f, -1.0f };
			transComp.rotation = { 17.f, 30.f, 0.f };
		}
	}

	void OnUpdate(float deltaTime) override {

		//处理队列事件
		ade::InputManager::GetInstance().ProcessEvents();

		// 更新相机控制器
		if (m_CameraController) {
			m_CameraController->Update(deltaTime);
		}

		// 旋转速度（度/秒）
		float rotationSpeed = 90.0f; // 每秒旋转90度

		// 更新 Cube 0 - 绕Y轴旋转
		if (mCubes[0] && mCubes[0]->HasComponent<ade::AdTransformComponent>()) {
			auto& transComp = mCubes[0]->GetComponent<ade::AdTransformComponent>();
			transComp.rotation.y += rotationSpeed * deltaTime;
			// 保持在0-360度范围内
			if (transComp.rotation.y >= 360.0f) {
				transComp.rotation.y -= 360.0f;
			}
		}

		// 更新 Cube 1 - 绕X轴旋转
		if (mCubes[1] && mCubes[1]->HasComponent<ade::AdTransformComponent>()) {
			auto& transComp = mCubes[1]->GetComponent<ade::AdTransformComponent>();
			transComp.rotation.x += rotationSpeed * deltaTime;
			if (transComp.rotation.x >= 360.0f) {
				transComp.rotation.x -= 360.0f;
			}
		}

		// 更新 Cube 2 - 绕Z轴旋转
		if (mCubes[2] && mCubes[2]->HasComponent<ade::AdTransformComponent>()) {
			auto& transComp = mCubes[2]->GetComponent<ade::AdTransformComponent>();
			transComp.rotation.z += rotationSpeed * deltaTime;
			if (transComp.rotation.z >= 360.0f) {
				transComp.rotation.z -= 360.0f;
			}
		}
		// 更新 Cube 3 - 绕Z轴旋转
		if (mCubes[3] && mCubes[3]->HasComponent<ade::AdTransformComponent>()) {
			auto& transComp = mCubes[3]->GetComponent<ade::AdTransformComponent>();
			transComp.rotation.z += rotationSpeed * deltaTime;
			
			if (transComp.rotation.z >= 360.0f) {
				transComp.rotation.z -= 360.0f;
			}
		}
	}

	/**
	 * @brief 场景销毁回调函数，当前为空实现。
	 *
	 * @param scene 指向即将销毁的场景对象的指针。
	 */
	void OnSceneDestroy(ade::AdScene* scene) override {

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
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		int32_t imageIndex;
		if (mRenderer->Begin(&imageIndex)) {
			mRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
		}

		VkCommandBuffer cmdBuffer = mCmdBuffers[imageIndex];
		ade::AdVKCommandPool::BeginCommandBuffer(cmdBuffer);

		mRenderTarget->Begin(cmdBuffer);
		mRenderTarget->RenderMaterialSystems(cmdBuffer);
		mRenderTarget->End(cmdBuffer);

		ade::AdVKCommandPool::EndCommandBuffer(cmdBuffer);
		if (mRenderer->End(imageIndex, { cmdBuffer })) {
			mRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
		}
	}

	/**
	 * @brief 应用程序销毁阶段，释放所有已创建的资源。
	 */
	void OnDestroy() override {
		ade::AdRenderContext* renderCxt = ade::AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		vkDeviceWaitIdle(device->GetHandle());
		mCubeMesh.reset();
		mCmdBuffers.clear();
		mRenderTarget.reset();
		mRenderPass.reset();
		mRenderer.reset();
	}

private:
	std::shared_ptr<ade::AdVKRenderPass> mRenderPass;       ///< 渲染通道对象
	std::shared_ptr<ade::AdRenderTarget> mRenderTarget;     ///< 渲染目标对象
	std::shared_ptr<ade::AdRenderer> mRenderer;             ///< 渲染器对象

	std::vector<VkCommandBuffer> mCmdBuffers;               ///< 命令缓冲区数组
	std::shared_ptr<ade::AdMesh> mCubeMesh;                 ///< 立方体网格对象
	std::vector<ade::AdEntity*> mCubes;

	std::unique_ptr<ade::AdCameraControllerManager> m_CameraController;

	bool m_MouseDragging = false;
	glm::vec2 m_LastMousePos;
	float m_CameraYaw = 0.0f;
	float m_CameraPitch = 0.0f;
	float m_CameraSensitivity = 0.1f;
	float m_CameraRadius = 2.0f;
	void SetupCameraControls() {
		ade::InputManager::GetInstance().Subscribe<ade::MouseClickEvent>(
			[this](ade::MouseClickEvent& event) {
				HandleMouseClick(event);
			}
		);

		ade::InputManager::GetInstance().Subscribe<ade::MouseMoveEvent>(
			[this](ade::MouseMoveEvent& event) {
				HandleMouseMove(event);
			}
		);

		ade::InputManager::GetInstance().Subscribe<ade::MouseReleaseEvent>(
			[this](ade::MouseReleaseEvent& event) {
				HandleMouseRelease(event);
			}
		);

		ade::InputManager::GetInstance().Subscribe<ade::MouseScrollEvent>(
			[this](ade::MouseScrollEvent& event) {
				HandleMouseScroll(event);
			}
		);

		ade::InputManager::GetInstance().Subscribe<ade::KeyPressEvent>(
			[this](ade::KeyPressEvent& event) {
				HandleKeyPress(event);
			}
		);

		ade::InputManager::GetInstance().Subscribe<ade::KeyReleaseEvent>(
			[this](ade::KeyReleaseEvent& event) {
				HandleKeyRelease(event);
			}
		);
	}

	void HandleMouseClick(ade::MouseClickEvent& event) {
		if (event.GetButton() == GLFW_MOUSE_BUTTON_LEFT) {
			m_MouseDragging = true;
			m_LastMousePos = event.GetPosition();  // 这里设置起点
		}
	}

	void HandleMouseRelease(ade::MouseReleaseEvent& event) {
		if (event.GetButton() == GLFW_MOUSE_BUTTON_LEFT) {
			m_MouseDragging = false;
		}
	}

	void HandleMouseMove(ade::MouseMoveEvent& event) {
		if (m_MouseDragging && m_CameraController) {
			glm::vec2 currentPos = event.GetPosition();
			glm::vec2 delta = currentPos - m_LastMousePos;

			// 调用相机控制器处理鼠标移动
			m_CameraController->OnMouseMove(delta.x, delta.y);

			m_LastMousePos = currentPos;
		}
	}

	void HandleMouseScroll(ade::MouseScrollEvent& event) {
		if (m_CameraController) {
			float delta = event.GetYOffset();
			m_CameraController->OnMouseScroll(delta);
		}
	}

	void HandleKeyPress(ade::KeyPressEvent& event) {
		if (m_CameraController) {
			ade::AdEntity* camera = m_CameraController->GetCameraEntity();
			if (camera && camera->HasComponent<ade::AdFirstPersonCameraComponent>()) {
				auto& fpCamera = camera->GetComponent<ade::AdFirstPersonCameraComponent>();

				switch (event.GetKey()) {
				case GLFW_KEY_W: fpCamera.SetMoveForward(true); break;
				case GLFW_KEY_S: fpCamera.SetMoveBackward(true); break;
				case GLFW_KEY_A: fpCamera.SetMoveLeft(true); break;
				case GLFW_KEY_D: fpCamera.SetMoveRight(true); break;
				}
			}
		}
	}

	void HandleKeyRelease(ade::KeyReleaseEvent& event) {
		if (m_CameraController) {
			ade::AdEntity* camera = m_CameraController->GetCameraEntity();
			if (camera && camera->HasComponent<ade::AdFirstPersonCameraComponent>()) {
				auto& fpCamera = camera->GetComponent<ade::AdFirstPersonCameraComponent>();

				switch (event.GetKey()) {
				case GLFW_KEY_W: fpCamera.SetMoveForward(false); break;
				case GLFW_KEY_S: fpCamera.SetMoveBackward(false); break;
				case GLFW_KEY_A: fpCamera.SetMoveLeft(false); break;
				case GLFW_KEY_D: fpCamera.SetMoveRight(false); break;
				}
			}
		}
	}

};

/**
 * @brief 入口点函数，用于创建应用程序实例。
 *
 * @return 返回指向新创建的 SandBoxApp 实例的指针。
 */
ade::AdApplication* CreateApplicationEntryPoint() {
	return new SandBoxApp();
}