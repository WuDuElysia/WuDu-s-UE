#define NOMINMAX
#include "Gui/AdBuiltinComponentRegistration.h"
#include "Gui/AdComponentRegistry.h"
#include "Gui/AdComponentInfo.h"
#include "Gui/AdPropertyDescriptor.h"
#include "ECS/Component/AdTransformComponent.h"
#include "ECS/Component/Material/AdPBRMaterialComponent.h"
#include "ECS/Component/Light/AdDirectionalLightComponent.h"
#include "ECS/Component/Light/AdPointLightComponent.h"
#include "ECS/Component/Light/AdSpotLightComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "entt/core/type_info.hpp"

namespace WuDu {

	// Helper: get the first AdPBRMaterial* from a PBR material component
	static AdPBRMaterial* GetFirstPBRMaterial(AdPBRMaterialComponent* comp) {
		const auto& meshMaterials = comp->GetMeshMaterials();
		if (meshMaterials.empty()) {
			return nullptr;
		}
		return meshMaterials.begin()->first;
	}

	static void RegisterTransformComponent() {
		AdComponentInfo info;
		info.typeId = entt::type_id<AdTransformComponent>().hash();
		info.displayName = "Transform";

		info.hasComponent = [](entt::registry& reg, entt::entity e) -> bool {
			return reg.all_of<AdTransformComponent>(e);
		};
		info.getComponent = [](entt::registry& reg, entt::entity e) -> void* {
			return &reg.get<AdTransformComponent>(e);
		};
		info.addComponent = [](entt::registry& reg, entt::entity e) {
			reg.emplace<AdTransformComponent>(e);
		};
		info.removeComponent = [](entt::registry& reg, entt::entity e) {
			reg.remove<AdTransformComponent>(e);
		};

		// position
		{
			AdPropertyDescriptor prop;
			prop.name = "position";
			prop.displayLabel = "Position";
			prop.type = PropertyType::Vec3;
			prop.dragSpeed = 0.1f;
			prop.getter = [](void* comp, void* out) {
				auto* tc = static_cast<AdTransformComponent*>(comp);
				*static_cast<glm::vec3*>(out) = tc->position;
			};
			prop.setter = [](void* comp, const void* in) {
				auto* tc = static_cast<AdTransformComponent*>(comp);
				tc->position = *static_cast<const glm::vec3*>(in);
			};
			info.properties.push_back(prop);
		}

		// rotation
		{
			AdPropertyDescriptor prop;
			prop.name = "rotation";
			prop.displayLabel = "Rotation";
			prop.type = PropertyType::Vec3;
			prop.dragSpeed = 0.1f;
			prop.getter = [](void* comp, void* out) {
				auto* tc = static_cast<AdTransformComponent*>(comp);
				*static_cast<glm::vec3*>(out) = tc->rotation;
			};
			prop.setter = [](void* comp, const void* in) {
				auto* tc = static_cast<AdTransformComponent*>(comp);
				tc->rotation = *static_cast<const glm::vec3*>(in);
			};
			info.properties.push_back(prop);
		}

		// scale
		{
			AdPropertyDescriptor prop;
			prop.name = "scale";
			prop.displayLabel = "Scale";
			prop.type = PropertyType::Vec3;
			prop.dragSpeed = 0.1f;
			prop.getter = [](void* comp, void* out) {
				auto* tc = static_cast<AdTransformComponent*>(comp);
				*static_cast<glm::vec3*>(out) = tc->scale;
			};
			prop.setter = [](void* comp, const void* in) {
				auto* tc = static_cast<AdTransformComponent*>(comp);
				tc->scale = *static_cast<const glm::vec3*>(in);
			};
			info.properties.push_back(prop);
		}

		AdComponentRegistry::GetInstance().RegisterComponent(info);
	}

	static void RegisterPBRMaterialComponent() {
		AdComponentInfo info;
		info.typeId = entt::type_id<AdPBRMaterialComponent>().hash();
		info.displayName = "PBR Material";

		info.hasComponent = [](entt::registry& reg, entt::entity e) -> bool {
			return reg.all_of<AdPBRMaterialComponent>(e);
		};
		info.getComponent = [](entt::registry& reg, entt::entity e) -> void* {
			return &reg.get<AdPBRMaterialComponent>(e);
		};
		info.addComponent = [](entt::registry& reg, entt::entity e) {
			reg.emplace<AdPBRMaterialComponent>(e);
		};
		info.removeComponent = [](entt::registry& reg, entt::entity e) {
			reg.remove<AdPBRMaterialComponent>(e);
		};

		// baseColorFactor
		{
			AdPropertyDescriptor prop;
			prop.name = "baseColorFactor";
			prop.displayLabel = "Base Color";
			prop.type = PropertyType::Color4;
			prop.getter = [](void* comp, void* out) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					*static_cast<glm::vec4*>(out) = mat->GetBaseColorFactor();
				}
			};
			prop.setter = [](void* comp, const void* in) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					mat->SetBaseColorFactor(*static_cast<const glm::vec4*>(in));
				}
			};
			info.properties.push_back(prop);
		}

		// metallicFactor
		{
			AdPropertyDescriptor prop;
			prop.name = "metallicFactor";
			prop.displayLabel = "Metallic";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.maxValue = 1.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					*static_cast<float*>(out) = mat->GetMetallicFactor();
				}
			};
			prop.setter = [](void* comp, const void* in) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					mat->SetMetallicFactor(*static_cast<const float*>(in));
				}
			};
			info.properties.push_back(prop);
		}

		// roughnessFactor
		{
			AdPropertyDescriptor prop;
			prop.name = "roughnessFactor";
			prop.displayLabel = "Roughness";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.maxValue = 1.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					*static_cast<float*>(out) = mat->GetRoughnessFactor();
				}
			};
			prop.setter = [](void* comp, const void* in) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					mat->SetRoughnessFactor(*static_cast<const float*>(in));
				}
			};
			info.properties.push_back(prop);
		}

		// aoFactor
		{
			AdPropertyDescriptor prop;
			prop.name = "aoFactor";
			prop.displayLabel = "AO";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.maxValue = 1.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					*static_cast<float*>(out) = mat->GetAoFactor();
				}
			};
			prop.setter = [](void* comp, const void* in) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					mat->SetAoFactor(*static_cast<const float*>(in));
				}
			};
			info.properties.push_back(prop);
		}

		// emissiveFactor
		{
			AdPropertyDescriptor prop;
			prop.name = "emissiveFactor";
			prop.displayLabel = "Emissive";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.maxValue = 10.0f;
			prop.dragSpeed = 0.05f;
			prop.getter = [](void* comp, void* out) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					*static_cast<float*>(out) = mat->GetEmissiveFactor();
				}
			};
			prop.setter = [](void* comp, const void* in) {
				auto* mc = static_cast<AdPBRMaterialComponent*>(comp);
				AdPBRMaterial* mat = GetFirstPBRMaterial(mc);
				if (mat) {
					mat->SetEmissiveFactor(*static_cast<const float*>(in));
				}
			};
			info.properties.push_back(prop);
		}

		AdComponentRegistry::GetInstance().RegisterComponent(info);
	}

	static void RegisterDirectionalLightComponent() {
		AdComponentInfo info;
		info.typeId = entt::type_id<AdDirectionalLightComponent>().hash();
		info.displayName = "Directional Light";

		info.hasComponent = [](entt::registry& reg, entt::entity e) -> bool {
			return reg.all_of<AdDirectionalLightComponent>(e);
		};
		info.getComponent = [](entt::registry& reg, entt::entity e) -> void* {
			return &reg.get<AdDirectionalLightComponent>(e);
		};
		info.addComponent = [](entt::registry& reg, entt::entity e) {
			reg.emplace<AdDirectionalLightComponent>(e);
		};
		info.removeComponent = [](entt::registry& reg, entt::entity e) {
			reg.remove<AdDirectionalLightComponent>(e);
		};

		// color
		{
			AdPropertyDescriptor prop;
			prop.name = "color";
			prop.displayLabel = "Color";
			prop.type = PropertyType::Color3;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdDirectionalLightComponent*>(comp);
				*static_cast<glm::vec3*>(out) = lc->GetColor();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdDirectionalLightComponent*>(comp);
				lc->SetColor(*static_cast<const glm::vec3*>(in));
			};
			info.properties.push_back(prop);
		}

		// intensity
		{
			AdPropertyDescriptor prop;
			prop.name = "intensity";
			prop.displayLabel = "Intensity";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.05f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdDirectionalLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetIntensity();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdDirectionalLightComponent*>(comp);
				lc->SetIntensity(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// enabled
		{
			AdPropertyDescriptor prop;
			prop.name = "enabled";
			prop.displayLabel = "Enabled";
			prop.type = PropertyType::Bool;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdDirectionalLightComponent*>(comp);
				*static_cast<bool*>(out) = lc->IsEnabled();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdDirectionalLightComponent*>(comp);
				lc->SetEnabled(*static_cast<const bool*>(in));
			};
			info.properties.push_back(prop);
		}

		// direction
		{
			AdPropertyDescriptor prop;
			prop.name = "direction";
			prop.displayLabel = "Direction";
			prop.type = PropertyType::Vec3;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdDirectionalLightComponent*>(comp);
				*static_cast<glm::vec3*>(out) = lc->GetDirection();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdDirectionalLightComponent*>(comp);
				lc->SetDirection(*static_cast<const glm::vec3*>(in));
			};
			info.properties.push_back(prop);
		}

		AdComponentRegistry::GetInstance().RegisterComponent(info);
	}

	static void RegisterPointLightComponent() {
		AdComponentInfo info;
		info.typeId = entt::type_id<AdPointLightComponent>().hash();
		info.displayName = "Point Light";

		info.hasComponent = [](entt::registry& reg, entt::entity e) -> bool {
			return reg.all_of<AdPointLightComponent>(e);
		};
		info.getComponent = [](entt::registry& reg, entt::entity e) -> void* {
			return &reg.get<AdPointLightComponent>(e);
		};
		info.addComponent = [](entt::registry& reg, entt::entity e) {
			reg.emplace<AdPointLightComponent>(e);
		};
		info.removeComponent = [](entt::registry& reg, entt::entity e) {
			reg.remove<AdPointLightComponent>(e);
		};

		// color
		{
			AdPropertyDescriptor prop;
			prop.name = "color";
			prop.displayLabel = "Color";
			prop.type = PropertyType::Color3;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				*static_cast<glm::vec3*>(out) = lc->GetColor();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				lc->SetColor(*static_cast<const glm::vec3*>(in));
			};
			info.properties.push_back(prop);
		}

		// intensity
		{
			AdPropertyDescriptor prop;
			prop.name = "intensity";
			prop.displayLabel = "Intensity";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.05f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetIntensity();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				lc->SetIntensity(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// enabled
		{
			AdPropertyDescriptor prop;
			prop.name = "enabled";
			prop.displayLabel = "Enabled";
			prop.type = PropertyType::Bool;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				*static_cast<bool*>(out) = lc->IsEnabled();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				lc->SetEnabled(*static_cast<const bool*>(in));
			};
			info.properties.push_back(prop);
		}

		// range
		{
			AdPropertyDescriptor prop;
			prop.name = "range";
			prop.displayLabel = "Range";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.1f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetRange();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				lc->SetRange(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// attenuationConstant
		{
			AdPropertyDescriptor prop;
			prop.name = "attenuationConstant";
			prop.displayLabel = "Attenuation Constant";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetAttenuationConstant();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				float val = *static_cast<const float*>(in);
				lc->SetAttenuation(val, lc->GetAttenuationLinear(), lc->GetAttenuationQuadratic());
			};
			info.properties.push_back(prop);
		}

		// attenuationLinear
		{
			AdPropertyDescriptor prop;
			prop.name = "attenuationLinear";
			prop.displayLabel = "Attenuation Linear";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetAttenuationLinear();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				float val = *static_cast<const float*>(in);
				lc->SetAttenuation(lc->GetAttenuationConstant(), val, lc->GetAttenuationQuadratic());
			};
			info.properties.push_back(prop);
		}

		// attenuationQuadratic
		{
			AdPropertyDescriptor prop;
			prop.name = "attenuationQuadratic";
			prop.displayLabel = "Attenuation Quadratic";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetAttenuationQuadratic();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdPointLightComponent*>(comp);
				float val = *static_cast<const float*>(in);
				lc->SetAttenuation(lc->GetAttenuationConstant(), lc->GetAttenuationLinear(), val);
			};
			info.properties.push_back(prop);
		}

		AdComponentRegistry::GetInstance().RegisterComponent(info);
	}

	static void RegisterSpotLightComponent() {
		AdComponentInfo info;
		info.typeId = entt::type_id<AdSpotLightComponent>().hash();
		info.displayName = "Spot Light";

		info.hasComponent = [](entt::registry& reg, entt::entity e) -> bool {
			return reg.all_of<AdSpotLightComponent>(e);
		};
		info.getComponent = [](entt::registry& reg, entt::entity e) -> void* {
			return &reg.get<AdSpotLightComponent>(e);
		};
		info.addComponent = [](entt::registry& reg, entt::entity e) {
			reg.emplace<AdSpotLightComponent>(e);
		};
		info.removeComponent = [](entt::registry& reg, entt::entity e) {
			reg.remove<AdSpotLightComponent>(e);
		};

		// color
		{
			AdPropertyDescriptor prop;
			prop.name = "color";
			prop.displayLabel = "Color";
			prop.type = PropertyType::Color3;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<glm::vec3*>(out) = lc->GetColor();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				lc->SetColor(*static_cast<const glm::vec3*>(in));
			};
			info.properties.push_back(prop);
		}

		// intensity
		{
			AdPropertyDescriptor prop;
			prop.name = "intensity";
			prop.displayLabel = "Intensity";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.05f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetIntensity();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				lc->SetIntensity(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// enabled
		{
			AdPropertyDescriptor prop;
			prop.name = "enabled";
			prop.displayLabel = "Enabled";
			prop.type = PropertyType::Bool;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<bool*>(out) = lc->IsEnabled();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				lc->SetEnabled(*static_cast<const bool*>(in));
			};
			info.properties.push_back(prop);
		}

		// range
		{
			AdPropertyDescriptor prop;
			prop.name = "range";
			prop.displayLabel = "Range";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.1f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetRange();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				lc->SetRange(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// innerCutoff (radians)
		{
			AdPropertyDescriptor prop;
			prop.name = "innerCutoff";
			prop.displayLabel = "Inner Cutoff";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.maxValue = 3.14159f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetSpotInnerCutoff();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				float val = *static_cast<const float*>(in);
				lc->SetSpotCutoff(val, lc->GetSpotOuterCutoff());
			};
			info.properties.push_back(prop);
		}

		// outerCutoff (radians)
		{
			AdPropertyDescriptor prop;
			prop.name = "outerCutoff";
			prop.displayLabel = "Outer Cutoff";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.maxValue = 3.14159f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetSpotOuterCutoff();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				float val = *static_cast<const float*>(in);
				lc->SetSpotCutoff(lc->GetSpotInnerCutoff(), val);
			};
			info.properties.push_back(prop);
		}

		// attenuationConstant
		{
			AdPropertyDescriptor prop;
			prop.name = "attenuationConstant";
			prop.displayLabel = "Attenuation Constant";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetAttenuationConstant();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				float val = *static_cast<const float*>(in);
				lc->SetAttenuation(val, lc->GetAttenuationLinear(), lc->GetAttenuationQuadratic());
			};
			info.properties.push_back(prop);
		}

		// attenuationLinear
		{
			AdPropertyDescriptor prop;
			prop.name = "attenuationLinear";
			prop.displayLabel = "Attenuation Linear";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetAttenuationLinear();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				float val = *static_cast<const float*>(in);
				lc->SetAttenuation(lc->GetAttenuationConstant(), val, lc->GetAttenuationQuadratic());
			};
			info.properties.push_back(prop);
		}

		// attenuationQuadratic
		{
			AdPropertyDescriptor prop;
			prop.name = "attenuationQuadratic";
			prop.displayLabel = "Attenuation Quadratic";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				*static_cast<float*>(out) = lc->GetAttenuationQuadratic();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* lc = static_cast<AdSpotLightComponent*>(comp);
				float val = *static_cast<const float*>(in);
				lc->SetAttenuation(lc->GetAttenuationConstant(), lc->GetAttenuationLinear(), val);
			};
			info.properties.push_back(prop);
		}

		AdComponentRegistry::GetInstance().RegisterComponent(info);
	}

	static void RegisterFirstPersonCameraComponent() {
		AdComponentInfo info;
		info.typeId = entt::type_id<AdFirstPersonCameraComponent>().hash();
		info.displayName = "First Person Camera";

		info.hasComponent = [](entt::registry& reg, entt::entity e) -> bool {
			return reg.all_of<AdFirstPersonCameraComponent>(e);
		};
		info.getComponent = [](entt::registry& reg, entt::entity e) -> void* {
			return &reg.get<AdFirstPersonCameraComponent>(e);
		};
		info.addComponent = [](entt::registry& reg, entt::entity e) {
			reg.emplace<AdFirstPersonCameraComponent>(e);
		};
		info.removeComponent = [](entt::registry& reg, entt::entity e) {
			reg.remove<AdFirstPersonCameraComponent>(e);
		};

		// NOTE: AdFirstPersonCameraComponent has private members (mFov, mNearPlane, mFarPlane,
		// mSensitivity, mMoveSpeed) with setters but no public getters. The getters below
		// cannot read the current values. We provide setters that work correctly, but the
		// inspector will not reflect the current state of these properties until public
		// getters are added to the component class.

		// fov (setter-only, no public getter)
		{
			AdPropertyDescriptor prop;
			prop.name = "fov";
			prop.displayLabel = "FOV";
			prop.type = PropertyType::Float;
			prop.minValue = 1.0f;
			prop.maxValue = 179.0f;
			prop.dragSpeed = 0.5f;
			prop.getter = [](void* comp, void* out) {
				// No public getter available - return default value
				*static_cast<float*>(out) = 65.0f;
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdFirstPersonCameraComponent*>(comp);
				cc->SetFov(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// nearPlane (setter-only, no public getter)
		{
			AdPropertyDescriptor prop;
			prop.name = "nearPlane";
			prop.displayLabel = "Near Plane";
			prop.type = PropertyType::Float;
			prop.minValue = 0.01f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				// No public getter available - return default value
				*static_cast<float*>(out) = 0.3f;
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdFirstPersonCameraComponent*>(comp);
				cc->SetNearPlane(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// farPlane (setter-only, no public getter)
		{
			AdPropertyDescriptor prop;
			prop.name = "farPlane";
			prop.displayLabel = "Far Plane";
			prop.type = PropertyType::Float;
			prop.minValue = 1.0f;
			prop.dragSpeed = 1.0f;
			prop.getter = [](void* comp, void* out) {
				// No public getter available - return default value
				*static_cast<float*>(out) = 1000.0f;
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdFirstPersonCameraComponent*>(comp);
				cc->SetFarPlane(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// sensitivity (setter-only, no public getter)
		{
			AdPropertyDescriptor prop;
			prop.name = "sensitivity";
			prop.displayLabel = "Sensitivity";
			prop.type = PropertyType::Float;
			prop.minValue = 0.01f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				// No public getter available - return default value
				*static_cast<float*>(out) = 0.2f;
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdFirstPersonCameraComponent*>(comp);
				cc->SetSensitivity(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// moveSpeed (setter-only, no public getter)
		{
			AdPropertyDescriptor prop;
			prop.name = "moveSpeed";
			prop.displayLabel = "Move Speed";
			prop.type = PropertyType::Float;
			prop.minValue = 0.0f;
			prop.dragSpeed = 0.1f;
			prop.getter = [](void* comp, void* out) {
				// No public getter available - return default value
				*static_cast<float*>(out) = 3.0f;
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdFirstPersonCameraComponent*>(comp);
				cc->SetMoveSpeed(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		AdComponentRegistry::GetInstance().RegisterComponent(info);
	}

	static void RegisterLookAtCameraComponent() {
		AdComponentInfo info;
		info.typeId = entt::type_id<AdLookAtCameraComponent>().hash();
		info.displayName = "LookAt Camera";

		info.hasComponent = [](entt::registry& reg, entt::entity e) -> bool {
			return reg.all_of<AdLookAtCameraComponent>(e);
		};
		info.getComponent = [](entt::registry& reg, entt::entity e) -> void* {
			return &reg.get<AdLookAtCameraComponent>(e);
		};
		info.addComponent = [](entt::registry& reg, entt::entity e) {
			reg.emplace<AdLookAtCameraComponent>(e);
		};
		info.removeComponent = [](entt::registry& reg, entt::entity e) {
			reg.remove<AdLookAtCameraComponent>(e);
		};

		// fov
		{
			AdPropertyDescriptor prop;
			prop.name = "fov";
			prop.displayLabel = "FOV";
			prop.type = PropertyType::Float;
			prop.minValue = 1.0f;
			prop.maxValue = 179.0f;
			prop.dragSpeed = 0.5f;
			prop.getter = [](void* comp, void* out) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				*static_cast<float*>(out) = cc->GetFov();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				cc->SetFov(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// nearPlane
		{
			AdPropertyDescriptor prop;
			prop.name = "nearPlane";
			prop.displayLabel = "Near Plane";
			prop.type = PropertyType::Float;
			prop.minValue = 0.01f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				*static_cast<float*>(out) = cc->GetNearPlane();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				cc->SetNearPlane(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// farPlane
		{
			AdPropertyDescriptor prop;
			prop.name = "farPlane";
			prop.displayLabel = "Far Plane";
			prop.type = PropertyType::Float;
			prop.minValue = 1.0f;
			prop.dragSpeed = 1.0f;
			prop.getter = [](void* comp, void* out) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				*static_cast<float*>(out) = cc->GetFarPlane();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				cc->SetFarPlane(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// radius
		{
			AdPropertyDescriptor prop;
			prop.name = "radius";
			prop.displayLabel = "Radius";
			prop.type = PropertyType::Float;
			prop.minValue = 0.1f;
			prop.dragSpeed = 0.1f;
			prop.getter = [](void* comp, void* out) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				*static_cast<float*>(out) = cc->GetRadius();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				cc->SetRadius(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// sensitivity
		{
			AdPropertyDescriptor prop;
			prop.name = "sensitivity";
			prop.displayLabel = "Sensitivity";
			prop.type = PropertyType::Float;
			prop.minValue = 0.01f;
			prop.dragSpeed = 0.01f;
			prop.getter = [](void* comp, void* out) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				*static_cast<float*>(out) = cc->GetSensitivity();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				cc->SetSensitivity(*static_cast<const float*>(in));
			};
			info.properties.push_back(prop);
		}

		// target
		{
			AdPropertyDescriptor prop;
			prop.name = "target";
			prop.displayLabel = "Target";
			prop.type = PropertyType::Vec3;
			prop.dragSpeed = 0.1f;
			prop.getter = [](void* comp, void* out) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				*static_cast<glm::vec3*>(out) = cc->GetTarget();
			};
			prop.setter = [](void* comp, const void* in) {
				auto* cc = static_cast<AdLookAtCameraComponent*>(comp);
				cc->SetTarget(*static_cast<const glm::vec3*>(in));
			};
			info.properties.push_back(prop);
		}

		AdComponentRegistry::GetInstance().RegisterComponent(info);
	}

	void RegisterBuiltinComponents() {
		RegisterTransformComponent();
		RegisterPBRMaterialComponent();
		RegisterDirectionalLightComponent();
		RegisterPointLightComponent();
		RegisterSpotLightComponent();
		RegisterFirstPersonCameraComponent();
		RegisterLookAtCameraComponent();
	}

} // namespace WuDu
