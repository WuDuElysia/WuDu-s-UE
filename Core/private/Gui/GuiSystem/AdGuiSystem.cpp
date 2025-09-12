// AdGuiSystem.cpp - 使用您封装的API
#include "Gui/GuiSystem/AdGuiSystem.h"
#include "AdFileUtil.h"

#include "AdApplication.h"
#include "Graphic/AdVKRenderPass.h"
#include <Window/AdGlfwWindow.h>

namespace ade {
	void AdGuiSystem::OnInit() {
		// 初始化ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // 允许窗口脱离主窗口
		io.ConfigViewportsNoAutoMerge = true;  // 防止窗口自动合并
		io.ConfigViewportsNoTaskBarIcon = false;

		io.DisplaySize = ImVec2(1360.0f, 768.0f);  // 使用窗口实际大小
		ImGui::StyleColorsDark();

		// 1. 获取设备和上下文
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKDevice* device = renderCxt->GetDevice();
		auto vkContext = dynamic_cast<ade::AdVKGraphicContext*>(renderCxt->GetGraphicContext());
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();



		// 2. 创建描述符池
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
		};
		mImGuiDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 100, poolSizes);

		ade::AdApplication* appCxt = AdApplication::GetAppContext()->app;
		AdGlfwWindow* window = static_cast<AdGlfwWindow*>(appCxt->GetWindow());
		ImGui_ImplGlfw_InitForVulkan(window->GetGLFWWindow(), true);

		CreateGUIRenderPass();
		// 3. 填充ImGui_ImplVulkan_InitInfo
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vkContext->GetInstance();
		init_info.PhysicalDevice = vkContext->GetPhyDevice();
		init_info.Device = device->GetHandle();
		init_info.QueueFamily = vkContext->GetGraphicQueueFamilyInfo().queueFamilyIndex;
		init_info.Queue = device->GetGraphicQueue(vkContext->GetGraphicQueueFamilyInfo().queueFamilyIndex)->GetHandle();
		init_info.DescriptorPool = mImGuiDescriptorPool->GetHandle();
		init_info.MinImageCount = device->GetSettings().swapchainImageCount;
		init_info.ImageCount = device->GetSettings().swapchainImageCount;
		init_info.UseDynamicRendering = false;
		init_info.PipelineInfoMain.RenderPass = mGUIRenderPass->GetHandle(); // 使用传入的渲染通道
		init_info.PipelineInfoMain.Subpass = 0; // 使用第一个子通道
		init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // 设置多重采样，应与您的渲染通道设置一致
		init_info.PipelineInfoForViewports.RenderPass = mGUIRenderPass->GetHandle(); // 或专门为多视口创建的渲染通道
		init_info.PipelineInfoForViewports.Subpass = 0;
		init_info.PipelineInfoForViewports.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		// 4. 初始化ImGui Vulkan后端
		ImGui_ImplVulkan_Init(&init_info);



		// 6. 确保完成
		vkDeviceWaitIdle(device->GetHandle());

		SetupGuiControls();

	}

	void AdGuiSystem::CreateGUIRenderPass() {
		// 获取设备和交换链
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		auto device = renderCxt->GetDevice();
		auto swapchain = renderCxt->GetSwapchain();

		// 定义GUI专用的渲染附件 - 只使用颜色附件，单采样
		std::vector<ade::Attachment> attachments = {
		    {
			.format = swapchain->GetSurfaceInfo().surfaceFormat.format,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,        // 保留3D场景内容
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // 从主渲染通道的最终布局开始
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // 最终呈现布局
			.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
		    }
		};

		// 定义GUI子通道 - 只使用颜色附件
		std::vector<ade::RenderSubPass> subpasses = {
		    {
			.colorAttachments = { 0 },                  // 使用第一个颜色附件
			.sampleCount = VK_SAMPLE_COUNT_1_BIT        // 与附件采样数一致
		    }
		};

		// 创建GUI专用的渲染通道
		mGUIRenderPass = std::make_shared<ade::AdVKRenderPass>(device, attachments, subpasses);

		// 使用GUI专用渲染通道创建渲染目标
		mGUIRenderTarget = std::make_shared<ade::AdRenderTarget>(mGUIRenderPass.get());

		// 创建渲染器
		mGUIRenderer = std::make_shared<ade::AdRenderer>();

		// 设置清除值 - 使用透明清除以便在3D场景上叠加GUI
		mGUIRenderTarget->SetColorClearValue({ 0.0f, 0.0f, 0.0f, 0.0f });

		mGUICmdBuffers = device->GetDefaultCmdPool()->AllocateCommandBuffer(swapchain->GetImages().size());

	}


	void AdGuiSystem::OnRender() {

		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		int32_t imageIndex;
		if (mGUIRenderer->Begin(&imageIndex)) {
			mGUIRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
		}

		VkCommandBuffer cmdBuffer = mGUICmdBuffers[imageIndex];
		ade::AdVKCommandPool::BeginCommandBuffer(cmdBuffer);

		// 使用GUI专用的渲染目标进行渲染
		mGUIRenderTarget->Begin(cmdBuffer);

		// 渲染ImGui
		ImDrawData* draw_data = ImGui::GetDrawData();
		if (draw_data && draw_data->CmdListsCount > 0) {
			ImGui_ImplVulkan_RenderDrawData(draw_data, cmdBuffer);
		}

		mGUIRenderTarget->End(cmdBuffer);

		ade::AdVKCommandPool::EndCommandBuffer(cmdBuffer);
		if (mGUIRenderer->End(imageIndex, { cmdBuffer })) {
			mGUIRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
		}

		// 关键：处理多视口
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			// 更新和渲染所有平台窗口
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void AdGuiSystem::RebuildResources() {
		// 1. 销毁旧的GUI命令缓冲区
		if (!mGUICmdBuffers.empty()) {
			mGUICmdBuffers.clear();
		}

		// 2. 重建命令缓冲区
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		auto device = renderCxt->GetDevice();
		auto swapchain = renderCxt->GetSwapchain();
		mGUICmdBuffers = device->GetDefaultCmdPool()->AllocateCommandBuffer(swapchain->GetImages().size());

		// 3. 重建GUI渲染目标
		mGUIRenderTarget->SetExtent({ swapchain->GetWidth(), swapchain->GetHeight() });
	}
	void AdGuiSystem::BeginGui() {
		ImGui_ImplVulkan_NewFrame();      // Vulkan后端先调用
		ImGui_ImplGlfw_NewFrame();        // GLFW后端后调用
		ImGui::NewFrame();

		// 执行所有注册的GUI函数
		for (auto& guiFunc : mGuiFunctions) {
			guiFunc();
		}
	}

	void AdGuiSystem::EndGui() {
		ImGui::Render();
	}

	void AdGuiSystem::AddGuiFunction(const std::function<void()>& func) {
		mGuiFunctions.push_back(func);
	}

	bool AdGuiSystem::ProcessInput() {
		ImGuiIO& io = ImGui::GetIO();
		return io.WantCaptureMouse || io.WantCaptureKeyboard;
	}

	void AdGuiSystem::OnDestroy() {
		// 先关闭ImGui后端
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		// 然后清理Vulkan资源
		mImGuiDescriptorPool.reset();
		mGUIRenderer.reset();
		mGUIRenderTarget.reset();
		mGUIRenderPass.reset();

		// 最后清理命令缓冲区
		if (!mGUICmdBuffers.empty()) {
			mGUICmdBuffers.clear();
		}

		// 最后销毁ImGui上下文
		ImGui::DestroyContext();
	}

	void AdGuiSystem::SetupGuiControls() {
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

		/*ade::InputManager::GetInstance().Subscribe<ade::MouseScrollEvent>(
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
		);*/
	}

	void AdGuiSystem::HandleMouseClick(ade::MouseClickEvent& event) {
		ImGuiIO& io = ImGui::GetIO();

		// 将鼠标点击事件转换为ImGui的鼠标按钮索引
		int button = -1;
		if (event.GetButton() == GLFW_MOUSE_BUTTON_LEFT) {
			button = 0;
		}
		else if (event.GetButton() == GLFW_MOUSE_BUTTON_RIGHT) {
			button = 1;
		}
		else if (event.GetButton() == GLFW_MOUSE_BUTTON_MIDDLE) {
			button = 2;
		}

		if (button >= 0) {
			// 更新ImGui的鼠标状态
			io.MouseDown[button] = true;

			// 处理拖动开始
			if (button == 0) {  // 左键点击
				m_MouseDragging = true;
				m_LastMousePos = event.GetPosition();
			}
		}
	}

	// 鼠标释放事件处理
	void AdGuiSystem::HandleMouseRelease(ade::MouseReleaseEvent& event) {
		ImGuiIO& io = ImGui::GetIO();

		// 将鼠标释放事件转换为ImGui的鼠标按钮索引
		int button = -1;
		if (event.GetButton() == GLFW_MOUSE_BUTTON_LEFT) {
			button = 0;
		}
		else if (event.GetButton() == GLFW_MOUSE_BUTTON_RIGHT) {
			button = 1;
		}
		else if (event.GetButton() == GLFW_MOUSE_BUTTON_MIDDLE) {
			button = 2;
		}

		if (button >= 0) {
			// 更新ImGui的鼠标状态
			io.MouseDown[button] = false;

			// 处理拖动结束
			if (button == 0) {  // 左键释放
				m_MouseDragging = false;
			}
		}
	}

	//鼠标移动事件处理
	void AdGuiSystem::HandleMouseMove(ade::MouseMoveEvent& event) {
		ImGuiIO& io = ImGui::GetIO();
		glm::vec2 currentPos = event.GetPosition();

		// 更新ImGui的鼠标位置
		io.MousePos = ImVec2(currentPos.x, currentPos.y);

		// 处理拖动
		if (m_MouseDragging) {
			glm::vec2 delta = currentPos - m_LastMousePos;

			// 更新ImGui的鼠标增量
			io.MouseDelta.x = delta.x;
			io.MouseDelta.y = delta.y;

			// 处理窗口拖动（如果需要）
			// ImGui会自动处理窗口拖动，我们只需提供正确的输入

			m_LastMousePos = currentPos;
		}
		else {
			// 重置鼠标增量
			io.MouseDelta.x = 0.0f;
			io.MouseDelta.y = 0.0f;
		}
	}

	//鼠标滚轮事件处理
	void AdGuiSystem::HandleMouseScroll(ade::MouseScrollEvent& event) {
		ImGuiIO& io = ImGui::GetIO();

		// 更新ImGui的鼠标滚轮状态
		io.MouseWheel += event.GetYOffset();
		io.MouseWheelH += event.GetXOffset();  // 水平滚动
	}

	//实现场景编辑面板

	void AdGuiSystem::SetResources(
					AdScene* scene,
					AdEntity* activeCamera,
					AdMesh* cubeMesh,
					AdUnlitMaterial* defaultMaterial) {
		mScene = scene;
		mActiveCamera = activeCamera;
		mCubeMesh = cubeMesh;
		mDefaultMaterial = defaultMaterial;
	}
	void AdGuiSystem::AddSceneEditor() {
		AddGuiFunction([this]() {
			// 左侧：场景层级视图
			ImGui::Begin("Scene Hierarchy");
			ShowSceneHierarchy();
			ImGui::End();

			// 右侧：属性编辑器
			ImGui::Begin("Properties");
			ShowTransformEditor();
			ImGui::End();

			// 底部：3D视口
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
			HandleSceneViewport();
			ImGui::End();
			ImGui::PopStyleVar();
			});
	}

	void AdGuiSystem::ShowSceneHierarchy() {
		if (!mScene) {
			ImGui::Text("No scene set");
			return;
		}

		if (ImGui::Button("Add Cube")) {
			// 创建新实体
			AdEntity* cube = mScene->CreateEntity("Cube");

			// 添加材质组件并关联网格和材质
			auto& materialComp = cube->AddComponent<AdUnlitMaterialComponent>();
			materialComp.AddMesh(mCubeMesh, mDefaultMaterial);

			// 选中新创建的实体
			mSelectedEntity = cube;
		}

		ImGui::Separator();

		// 显示场景中所有实体
		entt::registry& reg = mScene->GetEcsRegistry();
		auto view = reg.view<AdTransformComponent>();
		// 如果没有符合条件的实体，则直接返回
		if (std::distance(view.begin(), view.end()) == 0) {
			return;
		}
		
		// 遍历实体
		for (auto entity : view) {
			// 将entt::entity转换为AdEntity*
			AdEntity* adEntity = mScene->GetEntity(entity);
			if (!adEntity) continue;

			// 获取实体名称（需要实现AdNameComponent或类似功能）
			std::string name = "Unnamed Entity";
			
			name = adEntity->GetName();
			

			ImGui::PushID(static_cast<int>(entity));
			bool isSelected = (adEntity == mSelectedEntity);
			if (ImGui::Selectable(name.c_str(), &isSelected)) {
				mSelectedEntity = adEntity;
			}
			ImGui::PopID();
		}
	}

	void AdGuiSystem::ShowTransformEditor() {
		if (!mSelectedEntity) {
			ImGui::Text("No entity selected");
			return;
		}

		// 检查实体是否有变换组件
		if (!mSelectedEntity->HasComponent<AdTransformComponent>()) {
			ImGui::Text("Selected entity has no transform");
			return;
		}

		auto& transform = mSelectedEntity->GetComponent<AdTransformComponent>();

		// 位置编辑
		ImGui::Text("Position");
		ImGui::DragFloat3("##Position", &transform.position[0], 0.1f);

		// 旋转编辑
		ImGui::Text("Rotation (degrees)");
		ImGui::DragFloat3("##Rotation", &transform.rotation[0], 0.5f, -360.0f, 360.0f);

		// 缩放编辑
		ImGui::Text("Scale");
		ImGui::DragFloat3("##Scale", &transform.scale[0], 0.01f, 0.01f);
	}

	

	// 实现3D视口交互

	// 1. 主视口处理
	void AdGuiSystem::HandleSceneViewport() {
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		// 记录视口尺寸
		if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
			mViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
		}

		// 绘制渲染纹理
		if (mRenderTexture) {
			ImGui::Image((ImTextureID)(uint64_t)mRenderTexture->GetImageView(), viewportPanelSize,
				ImVec2(0, 1), ImVec2(1, 0));
		}

		// 处理视口交互
		ImGui::SetItemAllowOverlap();
		if (ImGui::IsItemHovered()) {
			// 处理鼠标点击选择
			if (ImGui::IsMouseClicked(0) && !io.WantCaptureMouse) {
				SelectEntityAtMousePosition(io.MousePos.x, io.MousePos.y);
			}

			// 处理拖动变换
			if (mSelectedEntity && ImGui::IsMouseDown(0) && !io.WantCaptureMouse) {
				HandleTransformDrag(io.MousePos);
			}
		}
	}



	//2. 实现实体选择（射线检测）
	// AdGuiSystem.cpp
	void AdGuiSystem::SelectEntityAtMousePosition(float mouseX, float mouseY) {
		if (!mActiveCamera || !mScene) return;

		auto& cameraTransform = mActiveCamera->GetComponent<AdTransformComponent>();
		auto& cameraComp = mActiveCamera->GetComponent<AdFirstPersonCameraComponent>();

		// 1. 将屏幕坐标转换为NDC坐标 (-1到1)
		float ndcX = (mouseX / mViewportSize.x) * 2.0f - 1.0f;
		float ndcY = 1.0f - (mouseY / mViewportSize.y) * 2.0f; // 翻转Y轴

		// 2. 创建射线（从摄像机位置到屏幕点）
		glm::vec3 rayOrigin;
		glm::vec3 rayDirection;

		// 3. 使用摄像机信息计算射线
		cameraComp.Unproject(ndcX, ndcY, rayOrigin, rayDirection);

		// 4. 射线与场景中实体的碰撞检测
		float closestDistance = std::numeric_limits<float>::max();
		AdEntity* closestEntity = nullptr;

		entt::registry& reg = mScene->GetEcsRegistry();
		auto view = reg.view<AdTransformComponent, AdUnlitMaterialComponent>();

		

		view.each([&](auto entity, AdTransformComponent& transform, AdUnlitMaterialComponent& materialComp) {
			// 将entt::entity转换为AdEntity*
			AdEntity* adEntity = mScene->GetEntity(entity);
			if (!adEntity) return;

			float distance = RayIntersectsCube(rayOrigin, rayDirection, transform);

			if (distance > 0 && distance < closestDistance) {
				closestDistance = distance;
				closestEntity = adEntity;
			}
			});

		mSelectedEntity = closestEntity;
	}

	float AdGuiSystem::RayIntersectsCube(const glm::vec3& rayOrigin,
		const glm::vec3& rayDirection,
		const AdTransformComponent& transform) {
		// 简化的射线-立方体相交检测
		// 实际应用中应该使用更精确的算法

		// 1. 将射线转换到立方体的局部空间
		glm::mat4 worldToModel = glm::inverse(transform.GetTransform());
		glm::vec4 localRayOrigin = worldToModel * glm::vec4(rayOrigin, 1.0f);
		glm::vec4 localRayDir = worldToModel * glm::vec4(rayDirection, 0.0f);

		// 2. 检测与单位立方体的相交
		float tMin = -std::numeric_limits<float>::max();
		float tMax = std::numeric_limits<float>::max();

		// 检查X轴
		if (std::abs(localRayDir.x) > 1e-6) {
			float t1 = (-0.5f - localRayOrigin.x) / localRayDir.x;
			float t2 = (0.5f - localRayOrigin.x) / localRayDir.x;
			tMin = std::max(tMin, std::min(t1, t2));
			tMax = std::min(tMax, std::max(t1, t2));
		}

		// 检查Y轴
		if (std::abs(localRayDir.y) > 1e-6) {
			float t1 = (-0.5f - localRayOrigin.y) / localRayDir.y;
			float t2 = (0.5f - localRayOrigin.y) / localRayDir.y;
			tMin = std::max(tMin, std::min(t1, t2));
			tMax = std::min(tMax, std::max(t1, t2));
		}

		// 检查Z轴
		if (std::abs(localRayDir.z) > 1e-6) {
			float t1 = (-0.5f - localRayOrigin.z) / localRayDir.z;
			float t2 = (0.5f - localRayOrigin.z) / localRayDir.z;
			tMin = std::max(tMin, std::min(t1, t2));
			tMax = std::min(tMax, std::max(t1, t2));
		}

		if (tMax >= std::max(0.0f, tMin)) {
			return tMin > 0 ? tMin : tMax;
		}

		return -1.0f; // 无相交
	}




	//3. 实现变换拖动
	// AdGuiSystem.cpp
	void AdGuiSystem::HandleTransformDrag(const ImVec2& currentMousePos) {
		if (!mSelectedEntity || !mActiveCamera) return;

		auto& transform = mSelectedEntity->GetComponent<AdTransformComponent>();
		auto& cameraTransform = mActiveCamera->GetComponent<AdTransformComponent>();

		// 计算鼠标移动量
		glm::vec2 delta = {
		    currentMousePos.x - mLastMousePos.x,
		    currentMousePos.y - mLastMousePos.y
		};

		switch (mCurrentTransformMode) {
		case TransformMode::Translate: {
			// 基于摄像机方向计算移动方向
			glm::vec3 forward = cameraTransform.GetTransform() * glm::vec4(0, 0, -1, 0);
			glm::vec3 right = cameraTransform.GetTransform() * glm::vec4(1, 0, 0, 0);

			// 计算移动量（考虑摄像机距离）
			float distance = glm::distance(transform.position, cameraTransform.position);
			float sensitivity = 0.01f * distance;

			transform.position += right * (delta.x * sensitivity);
			transform.position += glm::vec3(0, 1, 0) * (delta.y * sensitivity);
			break;
		}
		case TransformMode::Rotate: {
			// 绕Y轴旋转（简化版）
			transform.rotation.y += delta.x * 0.5f;
			transform.rotation.x += delta.y * 0.5f;
			// 限制X轴旋转范围，避免万向锁
			transform.rotation.x = glm::clamp(transform.rotation.x, -89.0f, 89.0f);
			break;
		}
		case TransformMode::Scale: {
			// 统一缩放
			float scaleDelta = (delta.x - delta.y) * 0.01f;
			transform.scale += glm::vec3(scaleDelta);
			// 确保缩放不为负
			transform.scale = glm::max(transform.scale, glm::vec3(0.01f));
			break;
		}
		}

		mLastMousePos = glm::vec2(currentMousePos.x, currentMousePos.y);
	}

	void AdGuiSystem::SelectEntity(AdEntity* entity) {
		mSelectedEntity = entity;
		if (entity) {
			// 选中时记录初始鼠标位置，用于拖动
			ImGuiIO& io = ImGui::GetIO();
			mLastMousePos = { io.MousePos.x, io.MousePos.y };
		}
	}
}