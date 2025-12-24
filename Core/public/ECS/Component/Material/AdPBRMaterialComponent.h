#ifndef ADPBRMATERIALCOMPONENT_H
#define ADPBRMATERIALCOMPONENT_H

#include "AdMaterialComponent.h"
#include "Render/AdMaterial.h"

namespace WuDu {
	// PBR材质纹理枚举
	enum PBRMaterialTexture {
		PBR_MAT_BASE_COLOR,      // 基础颜色纹理
		PBR_MAT_NORMAL,          // 法线纹理
		PBR_MAT_METALLIC_ROUGHNESS, // 金属度-粗糙度纹理（R通道：粗糙度，B通道：金属度）
		PBR_MAT_AO,              // 环境光遮蔽纹理
		PBR_MAT_EMISSIVE         // 自发光纹理
	};

	struct FrameUbo {
		glm::mat4  projMat{ 1.f };
		glm::mat4  viewMat{ 1.f };
		alignas(8) glm::ivec2 resolution;
		alignas(4) uint32_t frameId;
		alignas(4) float time;
	};

	// PBR材质UBO结构体
	struct PBRMaterialUbo {
		alignas(16) glm::vec4 baseColorFactor;     // 基础颜色因子
		alignas(4) float metallicFactor;            // 金属度因子
		alignas(4) float roughnessFactor;           // 粗糙度因子
		alignas(4) float aoFactor;                  // AO因子
		alignas(4) float emissiveFactor;            // 自发光因子
		alignas(16) TextureParam baseColorTextureParam;     // 基础颜色纹理参数
		alignas(16) TextureParam normalTextureParam;         // 法线纹理参数
		alignas(16) TextureParam metallicRoughnessTextureParam; // 金属度-粗糙度纹理参数
		alignas(16) TextureParam aoTextureParam;              // AO纹理参数
		alignas(16) TextureParam emissiveTextureParam;         // 自发光纹理参数
	};

	// PBR材质类
	class AdPBRMaterial : public AdMaterial {
	public:
		// 获取材质参数
		const PBRMaterialUbo& GetParams() const { return mParams; }
		
		// 获取材质属性
		const glm::vec4& GetBaseColorFactor() const { return mParams.baseColorFactor; }
		float GetMetallicFactor() const { return mParams.metallicFactor; }
		float GetRoughnessFactor() const { return mParams.roughnessFactor; }
		float GetAoFactor() const { return mParams.aoFactor; }
		float GetEmissiveFactor() const { return mParams.emissiveFactor; }
		
		// 设置材质属性
		void SetBaseColorFactor(const glm::vec4& color) {
			mParams.baseColorFactor = color;
			bShouldFlushParams = true;
		}
		
		void SetMetallicFactor(float metallic) {
			mParams.metallicFactor = metallic;
			bShouldFlushParams = true;
		}
		
		void SetRoughnessFactor(float roughness) {
			mParams.roughnessFactor = roughness;
			bShouldFlushParams = true;
		}
		
		void SetAoFactor(float ao) {
			mParams.aoFactor = ao;
			bShouldFlushParams = true;
		}
		
		void SetEmissiveFactor(float emissive) {
			mParams.emissiveFactor = emissive;
			bShouldFlushParams = true;
		}
		
	private:
		PBRMaterialUbo mParams{};
	};

	// PBR材质组件类
	class AdPBRMaterialComponent : public AdMaterialComponent<AdPBRMaterial> {
	};
} // namespace WuDu

#endif // ADPBRMATERIALCOMPONENT_H