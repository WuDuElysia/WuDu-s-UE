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
 * @brief SandBoxApp ��̳��� ade::AdApplication��������ʾ���� ECS ��ʵ����Ⱦʾ����
 *
 * ����ʵ����Ӧ�ó����������ڵĸ����׶Σ��������á���ʼ����������������Ⱦ�����١�
 * ��Ҫ���ܰ�����
 * - ���ô��ڴ�С�ͱ���
 * - ��ʼ����Ⱦ��Դ����Ⱦͨ����Ŀ�ꡢ��������ȣ�
 * - �����������������ʵ��ĳ���
 * - ʵ��ÿ֡��Ⱦ�߼�
 * - ������Դ
 */
class SandBoxApp : public ade::AdApplication {
protected:
	/**
	 * @brief ����Ӧ�ó���Ļ������á�
	 *
	 * @param appSettings ָ�� AppSettings �����ָ�룬�������ô��ڿ�ȡ��߶Ⱥͱ��⡣
	 */
	void OnConfiguration(ade::AppSettings* appSettings) override {
		appSettings->width = 1360;
		appSettings->height = 768;
		appSettings->title = "04_ECS_Entity";
	}

	/**
	 * @brief Ӧ�ó����ʼ���׶Σ�������Ⱦ�����Դ��
	 *
	 * ������
	 * - ������Ⱦͨ������ɫ��������ȸ�����
	 * - ������ȾĿ�겢�������ֵ
	 * - ������Ⱦ�����������
	 * - ������������������
	 */
	void OnInit() override {
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		// ������Ⱦ��������ɫ��������ȸ���
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

		// ������ͨ����ʹ����ɫ��������ȸ���
		std::vector<ade::RenderSubPass> subpasses = {
		    {
			.colorAttachments = { 0 },
			.depthStencilAttachments = { 1 },
			.sampleCount = VK_SAMPLE_COUNT_4_BIT
		    }
		};

		// ������Ⱦͨ��
		mRenderPass = std::make_shared<ade::AdVKRenderPass>(device, attachments, subpasses);

		// ������ȾĿ�겢�������ֵ
		mRenderTarget = std::make_shared<ade::AdRenderTarget>(mRenderPass.get());
		mRenderTarget->SetColorClearValue({ 0.1f, 0.2f, 0.3f, 1.f });
		mRenderTarget->SetDepthStencilClearValue({ 1, 0 });
		mRenderTarget->AddMaterialSystem<ade::AdBaseMaterialSystem>();

		// ������Ⱦ��
		mRenderer = std::make_shared<ade::AdRenderer>();

		// �����������
		mCmdBuffers = device->GetDefaultCmdPool()->AllocateCommandBuffer(swapchain->GetImages().size());
		

		// ������������������
		std::vector<ade::AdVertex> vertices;
		std::vector<uint32_t> indices;
		ade::AdGeometryUtil::CreateCube(-0.3f, 0.3f, -0.3f, 0.3f, -0.3f, 0.3f, vertices, indices);
		mCubeMesh = std::make_shared<ade::AdMesh>(vertices, indices);

		//�����������
		SetupCameraControls();
	}

	/**
	 * @brief ��ʼ���������ݣ�����������Ͷ��������ʵ�塣
	 *
	 * @param scene ָ��ǰ���������ָ�롣
	 */
	void OnSceneInit(ade::AdScene* scene) override {
		// ���������ʵ�岢���������
		ade::AdEntity* camera = scene->CreateEntity("Editor Camera");
		auto& cameraComp = camera->AddComponent<ade::AdFirstPersonCameraComponent>();
		// ��ʼ�����������������
		m_CameraController = std::make_unique<ade::AdCameraControllerManager>(camera);
		m_CameraController->SetAspect(1360.0f / 768.0f);
		
		mRenderTarget->SetCamera(camera);

		// �������ֻ�������
		auto baseMat0 = ade::AdMaterialFactory::GetInstance()->CreateMaterial<ade::AdBaseMaterial>();
		baseMat0->colorType = ade::COLOR_TYPE_NORMAL;
		auto baseMat1 = ade::AdMaterialFactory::GetInstance()->CreateMaterial<ade::AdBaseMaterial>();
		baseMat1->colorType = ade::COLOR_TYPE_TEXCOORD;

		
		// �������������ʵ�壬����������ʺͱ任����
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

		//��������¼�
		ade::InputManager::GetInstance().ProcessEvents();

		// �������������
		if (m_CameraController) {
			m_CameraController->Update(deltaTime);
		}

		// ��ת�ٶȣ���/�룩
		float rotationSpeed = 90.0f; // ÿ����ת90��

		// ���� Cube 0 - ��Y����ת
		if (mCubes[0] && mCubes[0]->HasComponent<ade::AdTransformComponent>()) {
			auto& transComp = mCubes[0]->GetComponent<ade::AdTransformComponent>();
			transComp.rotation.y += rotationSpeed * deltaTime;
			// ������0-360�ȷ�Χ��
			if (transComp.rotation.y >= 360.0f) {
				transComp.rotation.y -= 360.0f;
			}
		}

		// ���� Cube 1 - ��X����ת
		if (mCubes[1] && mCubes[1]->HasComponent<ade::AdTransformComponent>()) {
			auto& transComp = mCubes[1]->GetComponent<ade::AdTransformComponent>();
			transComp.rotation.x += rotationSpeed * deltaTime;
			if (transComp.rotation.x >= 360.0f) {
				transComp.rotation.x -= 360.0f;
			}
		}

		// ���� Cube 2 - ��Z����ת
		if (mCubes[2] && mCubes[2]->HasComponent<ade::AdTransformComponent>()) {
			auto& transComp = mCubes[2]->GetComponent<ade::AdTransformComponent>();
			transComp.rotation.z += rotationSpeed * deltaTime;
			if (transComp.rotation.z >= 360.0f) {
				transComp.rotation.z -= 360.0f;
			}
		}
		// ���� Cube 3 - ��Z����ת
		if (mCubes[3] && mCubes[3]->HasComponent<ade::AdTransformComponent>()) {
			auto& transComp = mCubes[3]->GetComponent<ade::AdTransformComponent>();
			transComp.rotation.z += rotationSpeed * deltaTime;
			
			if (transComp.rotation.z >= 360.0f) {
				transComp.rotation.z -= 360.0f;
			}
		}
	}

	/**
	 * @brief �������ٻص���������ǰΪ��ʵ�֡�
	 *
	 * @param scene ָ�򼴽����ٵĳ��������ָ�롣
	 */
	void OnSceneDestroy(ade::AdScene* scene) override {

	}

	/**
	 * @brief ÿ֡��Ⱦ�ص�������ִ����Ⱦ���̡�
	 *
	 * ������
	 * - ��ȡ������ͼ������
	 * - ��ʼ��¼�������
	 * - ִ����ȾĿ�����Ⱦ����
	 * - �ύ�������������ͼ��
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
	 * @brief Ӧ�ó������ٽ׶Σ��ͷ������Ѵ�������Դ��
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
	std::shared_ptr<ade::AdVKRenderPass> mRenderPass;       ///< ��Ⱦͨ������
	std::shared_ptr<ade::AdRenderTarget> mRenderTarget;     ///< ��ȾĿ�����
	std::shared_ptr<ade::AdRenderer> mRenderer;             ///< ��Ⱦ������

	std::vector<VkCommandBuffer> mCmdBuffers;               ///< �����������
	std::shared_ptr<ade::AdMesh> mCubeMesh;                 ///< �������������
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
			m_LastMousePos = event.GetPosition();  // �����������
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

			// ���������������������ƶ�
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
 * @brief ��ڵ㺯�������ڴ���Ӧ�ó���ʵ����
 *
 * @return ����ָ���´����� SandBoxApp ʵ����ָ�롣
 */
ade::AdApplication* CreateApplicationEntryPoint() {
	return new SandBoxApp();
}