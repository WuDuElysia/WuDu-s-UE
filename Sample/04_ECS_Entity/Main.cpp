#include "AdEntryPoint.h"
#include "AdFileUtil.h"
#include "Render/AdRenderTarget.h"
#include "Render/AdMesh.h"
#include "Render/AdRenderer.h"
#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKCommandBuffer.h"

#include "ECS/AdEntity.h"
#include "ECS/System/AdBaseMaterialSystem.h"
#include "ECS/Component/AdLookAtCameraComponent.h"

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
		/*mCmdBuffers = device->GetDefaultCmdPool()->AllocateCommandBuffer(4);*/

		// ������������������
		std::vector<ade::AdVertex> vertices;
		std::vector<uint32_t> indices;
		ade::AdGeometryUtil::CreateCube(-0.3f, 0.3f, -0.3f, 0.3f, -0.3f, 0.3f, vertices, indices);
		mCubeMesh = std::make_shared<ade::AdMesh>(vertices, indices);
	}

	/**
	 * @brief ��ʼ���������ݣ�����������Ͷ��������ʵ�塣
	 *
	 * @param scene ָ��ǰ���������ָ�롣
	 */
	void OnSceneInit(ade::AdScene* scene) override {
		// ���������ʵ�岢���������
		ade::AdEntity* camera = scene->CreateEntity("Editor Camera");
		auto& cameraComp = camera->AddComponent<ade::AdLookAtCameraComponent>();
		cameraComp.SetRadius(2.f);
		mRenderTarget->SetCamera(camera);

		// �������ֻ�������
		auto baseMat0 = ade::AdMaterialFactory::GetInstance()->CreateMaterial<ade::AdBaseMaterial>();
		baseMat0->colorType = ade::COLOR_TYPE_NORMAL;
		auto baseMat1 = ade::AdMaterialFactory::GetInstance()->CreateMaterial<ade::AdBaseMaterial>();
		baseMat1->colorType = ade::COLOR_TYPE_TEXCOORD;

		// �������������ʵ�壬����������ʺͱ任����
		{
			ade::AdEntity* cube = scene->CreateEntity("Cube 0");
			auto& materialComp = cube->AddComponent<ade::AdBaseMaterialComponent>();
			materialComp.AddMesh(mCubeMesh.get(), baseMat1);
			auto& transComp = cube->GetComponent<ade::AdTransformComponent>();
			transComp.scale = { 1.f, 1.f, 1.f };
			transComp.position = { 0.f, 0.f, 0.0f };
			transComp.rotation = { 17.f, 30.f, 0.f };
		}
		{
			ade::AdEntity* cube = scene->CreateEntity("Cube 1");
			auto& materialComp = cube->AddComponent<ade::AdBaseMaterialComponent>();
			materialComp.AddMesh(mCubeMesh.get(), baseMat0);
			auto& transComp = cube->GetComponent<ade::AdTransformComponent>();
			transComp.scale = { 0.5f, 0.5f, 0.5f };
			transComp.position = { -1.f, 0.f, 0.0f };
			transComp.rotation = { 17.f, 30.f, 0.f };
		}
		{
			ade::AdEntity* cube = scene->CreateEntity("Cube 2");
			auto& materialComp = cube->AddComponent<ade::AdBaseMaterialComponent>();
			materialComp.AddMesh(mCubeMesh.get(), baseMat1);
			auto& transComp = cube->GetComponent<ade::AdTransformComponent>();
			transComp.scale = { 0.5f, 0.5f, 0.5f };
			transComp.position = { 1.f, 0.f, 0.0f };
			transComp.rotation = { 17.f, 30.f, 0.f };

			/*ade::AdEntity* cube = scene->CreateEntity("Cube 3");
			auto& materialComp = cube->AddComponent<ade::AdBaseMaterialComponent>();
			materialComp.AddMesh(mCubeMesh.get(), baseMat0);
			auto& transComp = cube->GetComponent<ade::AdTransformComponent>();
			transComp.scale = { 0.5f, 0.5f, 0.5f };
			transComp.position = { 0.f, -1.f, 0.0f };
			transComp.rotation = { 17.f, 30.f, 0.f };*/
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
};

/**
 * @brief ��ڵ㺯�������ڴ���Ӧ�ó���ʵ����
 *
 * @return ����ָ���´����� SandBoxApp ʵ����ָ�롣
 */
ade::AdApplication* CreateApplicationEntryPoint() {
	return new SandBoxApp();
}