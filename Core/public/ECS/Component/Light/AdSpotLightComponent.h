#pragma once

#include "AdLightComponent.h"

namespace WuDu {

	//聚光灯组件
	class AdSpotLightComponent : public AdLightComponent {
	public:
                AdSpotLightComponent() : AdLightComponent(), mRange(10.0f) {
                        // 设置默认衰减参数
                        mAttenuationConstant = 1.0f;
                        mAttenuationLinear = 0.14f;
                        mAttenuationQuadratic = 0.07f;

                        // 设置默认聚光灯角度（内圆锥角30度，外圆锥角45度）
                        SetSpotCutoff(glm::radians(30.0f), glm::radians(45.0f));
                }

                LightUbo GetLightUbo() const override {
                        LightUbo ubo{};

                        // 聚光灯的位置w分量为1，表示这是一个位置点
                        ubo.position = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                        ubo.directionAndRange = glm::vec4(0.0f, 0.0f, 0.0f, mRange);

                        // 设置光源颜色和强度
                        ubo.colorAndIntensity = glm::vec4(GetColor(), GetIntensity());

                        // 设置聚光灯特有参数
                        ubo.attenuationConstant = mAttenuationConstant;
                        ubo.attenuationLinear = mAttenuationLinear;
                        ubo.attenuationQuadratic = mAttenuationQuadratic;
                        ubo.spotInnerCutoff = mSpotInnerCutoff;
                        ubo.spotOuterCutoff = mSpotOuterCutoff;

                        // 设置光源类型和启用状态
                        ubo.type = static_cast<uint32_t>(LightType::LIGHT_TYPE_SPOT);
                        ubo.enabled = IsEnabled() ? 1 : 0;

                        return ubo;
                }

                // 聚光灯特有属性设置方法
                void SetRange(float range) { mRange = range; mDirty = true; }

                void SetAttenuation(float constant, float linear, float quadratic) {
                        mAttenuationConstant = constant;
                        mAttenuationLinear = linear;
                        mAttenuationQuadratic = quadratic;
                        mDirty = true;
                }

                void SetSpotCutoff(float innerCutoff, float outerCutoff) {
                        // 存储余弦值，避免在着色器中重复计算
                        mSpotInnerCutoff = cos(innerCutoff);
                        mSpotOuterCutoff = cos(outerCutoff);
                        mDirty = true;
                }

                // 获取聚光灯属性
                float GetRange() const { return mRange; }
                float GetSpotInnerCutoff() const { return acos(mSpotInnerCutoff); }
                float GetSpotOuterCutoff() const { return acos(mSpotOuterCutoff); }
                float GetSpotInnerCutoffCos() const { return mSpotInnerCutoff; }
                float GetSpotOuterCutoffCos() const { return mSpotOuterCutoff; }
                float GetAttenuationConstant() const { return mAttenuationConstant; }
                float GetAttenuationLinear() const { return mAttenuationLinear; }
                float GetAttenuationQuadratic() const { return mAttenuationQuadratic; }

        private:
                float mRange;                      // 光源范围
                float mAttenuationConstant;        // 衰减常数项
                float mAttenuationLinear;          // 衰减线性项
                float mAttenuationQuadratic;       // 衰减二次项
                float mSpotInnerCutoff;            // 内圆锥角（余弦值）
                float mSpotOuterCutoff;            // 外圆锥角（余弦值）
	};
}