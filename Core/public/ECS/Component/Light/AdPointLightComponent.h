#pragma
#include "AdLightComponent.h"

namespace WuDu {

	//点光组件
	class AdPointLightComponent : public AdLightComponent {
	public:
		//设置默认衰减参数
		AdPointLightComponent() : AdLightComponent(), mRange(10.0f) {
			// 设置默认衰减参数
			mAttenuationConstant = 1.0f;
			mAttenuationLinear = 0.14f;
			mAttenuationQuadratic = 0.07f;
		}

		LightUbo GetLightUbo() const override {
			LightUbo ubo{};

			// 获取实体变换组件，计算世界空间位置
			//auto& transformComp = GetEntity()->GetComponent<AdTransformComponent>();
			//glm::vec3 worldPos = transformComp.GetWorldPosition();

			// 点光的位置w分量为1，表示这是一个位置点
			ubo.position = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

			// 设置光源颜色和强度
			ubo.color = GetColor();
			ubo.intensity = GetIntensity();

			// 设置点光特有参数
			ubo.range = mRange;
			ubo.attenuationConstant = mAttenuationConstant;
			ubo.attenuationLinear = mAttenuationLinear;
			ubo.attenuationQuadratic = mAttenuationQuadratic;

			// 设置光源类型和启用状态
			ubo.type = static_cast<uint32_t>(LightType::LIGHT_TYPE_POINT);
			ubo.enabled = IsEnabled() ? 1 : 0;

			return ubo;
		}

		// 点光特有属性设置方法
		void SetRange(float range) { mRange = range; mDirty = true; }
		void SetAttenuation(float constant, float linear, float quadratic) {
			mAttenuationConstant = constant;
			mAttenuationLinear = linear;
			mAttenuationQuadratic = quadratic;
			mDirty = true;
		}

		// 获取点光属性
		float GetRange() const { return mRange; }
		float GetAttenuationConstant() const { return mAttenuationConstant; }
		float GetAttenuationLinear() const { return mAttenuationLinear; }
		float GetAttenuationQuadratic() const { return mAttenuationQuadratic; }

	private:
		float mRange;                      // 光源范围
		float mAttenuationConstant;        // 衰减常数项
		float mAttenuationLinear;          // 衰减线性项
		float mAttenuationQuadratic;       // 衰减二次项
	};
}