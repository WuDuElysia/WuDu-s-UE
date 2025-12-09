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

namespace WuDu {
	/**
	* @brief 初始化材质系统，创建渲染管线及相关资源。
	*
	* 该函数用于初始化 AdBaseMaterialSystem，设置着色器、顶点输入布局、
	* 渲染管线状态等，最终创建 Vulkan 图形管线。
	*
	* @param renderPass 指向 Vulkan 渲染通道的指针，用于管线创建时指定渲染目标格式。
	*/
	void AdBaseMaterialSystem::OnInit(AdVKRenderPass* renderPass) {
		WuDu::AdVKDevice* device = GetDevice();

		// 定义着色器布局，包括一个顶点着色器阶段的 Push Constant
		WuDu::ShaderLayout shaderLayout = {
		    .pushConstants = {
			{
			    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,  // Push Constant 用于顶点着色器
			    .offset = 0,                               // 偏移为 0
			    .size = sizeof(PushConstants)              // 大小为 PushConstants 结构体大小
			}
		    }
		};

		// 创建管线布局，加载顶点和片段着色器，并传入着色器布局
		mPipelineLayout = std::make_shared<AdVKPipelineLayout>(device,
			AD_RES_SHADER_DIR"01_hello_buffer.vert",
			AD_RES_SHADER_DIR"01_hello_buffer.frag",
			shaderLayout);

		// 定义顶点输入绑定描述：每个顶点数据的步长和输入频率
		std::vector<VkVertexInputBindingDescription> vertexBindings = {
		    {
			.binding = 0,                          // 绑定点为 0
			.stride = sizeof(AdVertex),            // 每个顶点的字节大小
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX // 每个顶点读取一次
		    }
		};

		// 定义顶点属性描述：位置、纹理坐标、法线等属性在顶点结构中的位置和格式
		std::vector<VkVertexInputAttributeDescription> vertexAttrs = {
		    {
			.location = 0,                         // 属性位置 0：顶点位置
			.binding = 0,                          // 来自绑定 0
			.format = VK_FORMAT_R32G32B32_SFLOAT,  // 3个32位浮点数
			.offset = offsetof(AdVertex, pos)      // 在 AdVertex 中的偏移
		    },
		    {
			.location = 1,                         // 属性位置 1：纹理坐标
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,     // 2个32位浮点数
			.offset = offsetof(AdVertex, tex)
		    },
		    {
			.location = 2,                         // 属性位置 2：法线
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,  // 3个32位浮点数
			.offset = offsetof(AdVertex, nor)
		    }
		};

		// 创建图形管线对象
		mPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPipelineLayout.get());

		// 设置顶点输入状态
		mPipeline->SetVertexInputState(vertexBindings, vertexAttrs);

		// 设置图元装配状态：三角形列表，并启用深度测试
		mPipeline->SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)->EnableDepthTest();

		// 设置动态状态：视口和裁剪区域将在命令缓冲区中动态设置
		mPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

		// 设置多重采样状态：4倍采样，禁用采样遮罩
		mPipeline->SetMultisampleState(VK_SAMPLE_COUNT_4_BIT, VK_FALSE);

		mPipeline->SetSubPassIndex(0);

		// 最终创建图形管线
		mPipeline->Create();
	}

	/**
	* @brief 渲染使用基础材质的实体
	*
	* 该函数负责渲染场景中所有带有 AdTransformComponent 和 AdBaseMaterialComponent 组件的实体。
	* 它会绑定渲染管线、设置视口和裁剪区域，并为每个实体设置相应的 Push Constants，
	* 最后调用网格的 Draw 方法进行绘制。
	*
	* @param cmdBuffer Vulkan 命令缓冲区，用于记录渲染命令
	* @param renderTarget 当前渲染目标，包含帧缓冲等信息
	*/
	void AdBaseMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {
		// 获取当前场景，如果不存在则直接返回
		AdScene* scene = GetScene();
		if (!scene) {
			return;
		}

		// 获取 ECS 注册表并筛选出具有 Transform 和 BaseMaterial 组件的实体
		entt::registry& reg = scene->GetEcsRegistry();
		auto view = reg.view<AdTransformComponent, AdBaseMaterialComponent>();

		// 如果没有符合条件的实体，则直接返回
		if (std::distance(view.begin(), view.end()) == 0) {
			return;
		}

		// 绑定当前渲染管线
		mPipeline->Bind(cmdBuffer);

		// 设置视口和裁剪区域
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

		// 获取投影矩阵和视图矩阵
		glm::mat4 projMat = GetProjMat(renderTarget);
		glm::mat4 viewMat = GetViewMat(renderTarget);

		// 遍历所有符合条件的实体并进行渲染
		view.each([this, &cmdBuffer, &projMat, &viewMat](const auto& e, const AdTransformComponent& transComp, const AdBaseMaterialComponent& materialComp) {
			// 获取该组件中的网格材质映射
			auto meshMaterials = materialComp.GetMeshMaterials();

			// 遍历每种材质及其对应的网格索引列表
			for (const auto& entry : meshMaterials) {
				AdBaseMaterial* material = entry.first;

				// 如果材质为空，则输出警告日志并跳过
				if (!material) {
					LOG_W("TODO: default material or error material ?");
					continue;
				}

				// 构造并设置 Push Constants 数据
				PushConstants pushConstants{
				    .matrix = projMat * viewMat * transComp.GetTransform(),
				    .colorType = static_cast<uint32_t>(material->colorType)
				};
				vkCmdPushConstants(cmdBuffer, mPipelineLayout->GetHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

				// 遍历与当前材质关联的所有网格索引，并执行绘制
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