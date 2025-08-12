#include "ECS/System/AdBaseMaterialSystem.h"
#include "AdFileUtil.h"
#include "AdGeometryUtil.h"
#include "AdApplication.h"

#include "Render/AdRenderContext.h"
#include "Render/AdRenderTarget.h"

#include "Graphic/AdVKPipeline.h"
#include "Graphic/AdVKFrameBuffer.h"

#include "ECS/AdEntity.h"
#include "ECS/Component/AdLookAtCameraComponent.h"

namespace ade {
	/**
	* @brief ��ʼ������ϵͳ��������Ⱦ���߼������Դ��
	*
	* �ú������ڳ�ʼ�� AdBaseMaterialSystem��������ɫ�����������벼�֡�
	* ��Ⱦ����״̬�ȣ����մ��� Vulkan ͼ�ι��ߡ�
	*
	* @param renderPass ָ�� Vulkan ��Ⱦͨ����ָ�룬���ڹ��ߴ���ʱָ����ȾĿ���ʽ��
	*/
	void AdBaseMaterialSystem::OnInit(AdVKRenderPass* renderPass) {
		ade::AdVKDevice* device = GetDevice();

		// ������ɫ�����֣�����һ��������ɫ���׶ε� Push Constant
		ade::ShaderLayout shaderLayout = {
		    .pushConstants = {
			{
			    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,  // Push Constant ���ڶ�����ɫ��
			    .offset = 0,                               // ƫ��Ϊ 0
			    .size = sizeof(PushConstants)              // ��СΪ PushConstants �ṹ���С
			}
		    }
		};

		// �������߲��֣����ض����Ƭ����ɫ������������ɫ������
		mPipelineLayout = std::make_shared<AdVKPipelineLayout>(device,
			AD_RES_SHADER_DIR"01_hello_buffer.vert",
			AD_RES_SHADER_DIR"01_hello_buffer.frag",
			shaderLayout);

		// ���嶥�������������ÿ���������ݵĲ���������Ƶ��
		std::vector<VkVertexInputBindingDescription> vertexBindings = {
		    {
			.binding = 0,                          // �󶨵�Ϊ 0
			.stride = sizeof(AdVertex),            // ÿ��������ֽڴ�С
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX // ÿ�������ȡһ��
		    }
		};

		// ���嶥������������λ�á��������ꡢ���ߵ������ڶ���ṹ�е�λ�ú͸�ʽ
		std::vector<VkVertexInputAttributeDescription> vertexAttrs = {
		    {
			.location = 0,                         // ����λ�� 0������λ��
			.binding = 0,                          // ���԰� 0
			.format = VK_FORMAT_R32G32B32_SFLOAT,  // 3��32λ������
			.offset = offsetof(AdVertex, pos)      // �� AdVertex �е�ƫ��
		    },
		    {
			.location = 1,                         // ����λ�� 1����������
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,     // 2��32λ������
			.offset = offsetof(AdVertex, tex)
		    },
		    {
			.location = 2,                         // ����λ�� 2������
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,  // 3��32λ������
			.offset = offsetof(AdVertex, nor)
		    }
		};

		// ����ͼ�ι��߶���
		mPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPipelineLayout.get());

		// ���ö�������״̬
		mPipeline->SetVertexInputState(vertexBindings, vertexAttrs);

		// ����ͼԪװ��״̬���������б���������Ȳ���
		mPipeline->SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)->EnableDepthTest();

		// ���ö�̬״̬���ӿںͲü���������������ж�̬����
		mPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

		// ���ö��ز���״̬��4�����������ò�������
		mPipeline->SetMultisampleState(VK_SAMPLE_COUNT_4_BIT, VK_FALSE);

		// ���մ���ͼ�ι���
		mPipeline->Create();
	}

	/**
	* @brief ��Ⱦʹ�û������ʵ�ʵ��
	*
	* �ú���������Ⱦ���������д��� AdTransformComponent �� AdBaseMaterialComponent �����ʵ�塣
	* �������Ⱦ���ߡ������ӿںͲü����򣬲�Ϊÿ��ʵ��������Ӧ�� Push Constants��
	* ����������� Draw �������л��ơ�
	*
	* @param cmdBuffer Vulkan ������������ڼ�¼��Ⱦ����
	* @param renderTarget ��ǰ��ȾĿ�꣬����֡�������Ϣ
	*/
	void AdBaseMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {
		// ��ȡ��ǰ�����������������ֱ�ӷ���
		AdScene* scene = GetScene();
		if (!scene) {
			return;
		}

		// ��ȡ ECS ע���ɸѡ������ Transform �� BaseMaterial �����ʵ��
		entt::registry& reg = scene->GetEcsRegistry();
		auto view = reg.view<AdTransformComponent, AdBaseMaterialComponent>();

		// ���û�з���������ʵ�壬��ֱ�ӷ���
		if (std::distance(view.begin(), view.end()) == 0) {
			return;
		}

		// �󶨵�ǰ��Ⱦ����
		mPipeline->Bind(cmdBuffer);

		// �����ӿںͲü�����
		AdVKFrameBuffer* frameBuffer = renderTarget->GetFrameBuffer();
		VkViewport viewport = {
		    .x = 0,
		    .y = 0,
		    .width = static_cast<float>(frameBuffer->GetWidth()),
		    .height = static_cast<float>(frameBuffer->GetHeight()),
		    .minDepth = 0.f,
		    .maxDepth = 1.f
		};
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = {
		    .offset = { 0, 0 },
		    .extent = { frameBuffer->GetWidth(), frameBuffer->GetHeight() }
		};
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		// ��ȡͶӰ�������ͼ����
		glm::mat4 projMat = GetProjMat(renderTarget);
		glm::mat4 viewMat = GetViewMat(renderTarget);

		// �������з���������ʵ�岢������Ⱦ
		view.each([this, &cmdBuffer, &projMat, &viewMat](const auto& e, const AdTransformComponent& transComp, const AdBaseMaterialComponent& materialComp) {
			// ��ȡ������е��������ӳ��
			auto meshMaterials = materialComp.GetMeshMaterials();

			// ����ÿ�ֲ��ʼ����Ӧ�����������б�
			for (const auto& entry : meshMaterials) {
				AdBaseMaterial* material = entry.first;

				// �������Ϊ�գ������������־������
				if (!material) {
					LOG_W("TODO: default material or error material ?");
					continue;
				}

				// ���첢���� Push Constants ����
				PushConstants pushConstants{
				    .matrix = projMat * viewMat * transComp.GetTransform(),
				    .colorType = static_cast<uint32_t>(material->colorType)
				};
				vkCmdPushConstants(cmdBuffer, mPipelineLayout->GetHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

				// �����뵱ǰ���ʹ���������������������ִ�л���
				for (const auto& meshIndex : entry.second) {
					AdMesh* mesh = materialComp.GetMesh(meshIndex);
					if (mesh) {
						mesh->Draw(cmdBuffer);
					}
				}
			}
			});
	}

	void AdBaseMaterialSystem::OnDestroy() {
		mPipeline.reset();
		mPipelineLayout.reset();
	}
}