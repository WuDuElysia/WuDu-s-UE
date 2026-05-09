#pragma once
#ifndef AD_LIGHT_COMPONENT_H
#define AD_LIGHT_COMPONENT_H

#include "ECS/AdComponent.h"
#include "ECS/AdEntity.h"
#include "ECS/Component/AdTransformComponent.h"
#include <glm/glm.hpp>

namespace WuDu{

    //光源类型
	enum class LightType {
		LIGHT_TYPE_DIRECTIONAL = 0,		//方向光
		LIGHT_TYPE_POINT,		//点光
		LIGHT_TYPE_SPOT		//聚光灯
	};

	//光源UBO结构体 - 匹配GLSL std140布局，使用vec4存储vec3+float
	struct LightUbo {
		alignas(16) glm::vec4 position;           // position.xyz, position.w = 0或1
		alignas(16) glm::vec4 directionAndRange;  // direction.xyz, range.w
		alignas(16) glm::vec4 colorAndIntensity;  // color.xyz, intensity.w

		alignas(4) float spotInnerCutoff;
		alignas(4) float spotOuterCutoff;
		alignas(4) float attenuationConstant;
		alignas(4) float attenuationLinear;

		alignas(4) float attenuationQuadratic;
		alignas(4) uint32_t type;
		alignas(4) uint32_t enabled;
		alignas(4) float padding;
	};

	// 光照UBO结构体
	struct LightingUbo {
		alignas(16) LightUbo lights[16];         // 支持最多16个光源
		alignas(16) glm::vec4 ambientColorAndIntensity;  // xyz: 环境光颜色, w: 环境光强度
		alignas(4) uint32_t numLights;            // 实际光源数量
		alignas(4) uint32_t padding0;             // 对齐填充
		alignas(4) uint32_t padding1;             // 对齐填充
		alignas(4) uint32_t padding2;             // 对齐填充
	};
	
	// 光源组件基类
	class AdLightComponent : public AdComponent {
	public:
		AdLightComponent() = default;
		virtual ~AdLightComponent() = default;

		// 组件生命周期方法
		void OnInit() {}
		void OnUpdate(float deltaTime) {}
		void OnDestroy() {}

		// 获取光源UBO数据（纯虚函数，子类必须实现）
		virtual LightUbo GetLightUbo() const = 0;

		// 光源属性设置方法
		void SetColor(const glm::vec3& color) { mColor = color; mDirty = true; }
		void SetIntensity(float intensity) { mIntensity = intensity; mDirty = true; }
		void SetEnabled(bool enabled) { mEnabled = enabled; mDirty = true; }

		// 获取光源属性
		const glm::vec3& GetColor() const { return mColor; }
		float GetIntensity() const { return mIntensity; }
		bool IsEnabled() const { return mEnabled; }

		// 检查光源是否需要更新
		bool IsDirty() const { return mDirty; }
		void ClearDirty() { mDirty = false; }

	protected:
		glm::vec3 mColor{ 1.0f, 1.0f, 1.0f };  // 光源颜色
		float mIntensity{ 1.0f };               // 光源强度
		bool mEnabled{ true };                  // 是否启用
		bool mDirty{ true };                    // 标记光源数据是否需要更新
	};

}

#endif