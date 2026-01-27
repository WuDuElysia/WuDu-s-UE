#pragma

#include "AdLightComponent.h"

namespace WuDu {
	
	//方向光组件
	class AdDirectionalLightComponent : public AdLightComponent {
	public:
		AdDirectionalLightComponent() : AdLightComponent(), mDirection(0.0f, -1.0f, 0.0f) {}

		LightUbo GetLightUbo() const override {
			LightUbo ubo{};

			// 方向光的位置w分量为0，表示这是一个方向向量
			ubo.position = glm::vec4(mDirection, 0.0f);
			ubo.direction = mDirection;

			// 设置光源颜色和强度
			ubo.color = GetColor();
			ubo.intensity = GetIntensity();

			// 设置光源类型和启用状态
			ubo.type = static_cast<uint32_t>(LightType::LIGHT_TYPE_DIRECTIONAL);
			ubo.enabled = IsEnabled() ? 1 : 0;

			return ubo;
		}

		// 方向光特有方法
		void SetDirection(const glm::vec3& direction) {
			mDirection = glm::normalize(direction);
			mDirty = true;
		}

		const glm::vec3& GetDirection() const { return mDirection; }
		
	private:
		glm::vec3 mDirection;  // 光源方向（已归一化）
	};
}