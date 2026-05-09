#define NOMINMAX
#include "Serialization/AdBuiltinComponentSerializers.h"
#include "Serialization/AdComponentSerializerRegistry.h"
#include "ECS/AdEntity.h"
#include "ECS/Component/AdTransformComponent.h"
#include "ECS/Component/Material/AdPBRMaterialComponent.h"
#include "ECS/Component/Light/AdDirectionalLightComponent.h"
#include "ECS/Component/Light/AdPointLightComponent.h"
#include "ECS/Component/Light/AdSpotLightComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "Render/AdMesh.h"
#include "Render/AdSampler.h"
#include "Resource/AdModelResource.h"
#include "AdFileUtil.h"
#include "AdLog.h"
#include <string>

namespace WuDu {

    // -----------------------------------------------------------------------
    // Helper: strip resource root prefix to get a relative path
    // -----------------------------------------------------------------------
    static std::string ToRelativePath(const std::string& absPath) {
        std::string root = AD_RES_ROOT_DIR;
        if (absPath.find(root) == 0) {
            return absPath.substr(root.size());
        }
        return absPath;
    }

    static std::string ToAbsolutePath(const std::string& relPath) {
        std::string root = AD_RES_ROOT_DIR;
        return root + relPath;
    }

    // -----------------------------------------------------------------------
    // 3.2.1  AdTransformComponent
    // -----------------------------------------------------------------------
    static nlohmann::json SerializeTransform(const AdComponent& comp) {
        const auto& t = static_cast<const AdTransformComponent&>(comp);
        return {
            { "position", { t.position.x, t.position.y, t.position.z } },
            { "rotation", { t.rotation.x, t.rotation.y, t.rotation.z } },
            { "scale",    { t.scale.x,    t.scale.y,    t.scale.z    } }
        };
    }

    static void DeserializeTransform(AdEntity* entity, const nlohmann::json& j, const nlohmann::json& /*resources*/) {
        auto& t = entity->HasComponent<AdTransformComponent>()
                    ? entity->GetComponent<AdTransformComponent>()
                    : entity->AddComponent<AdTransformComponent>();
        if (j.contains("position")) {
            auto& p = j["position"];
            t.position = { p[0].get<float>(), p[1].get<float>(), p[2].get<float>() };
        }
        if (j.contains("rotation")) {
            auto& r = j["rotation"];
            t.rotation = { r[0].get<float>(), r[1].get<float>(), r[2].get<float>() };
        }
        if (j.contains("scale")) {
            auto& s = j["scale"];
            t.scale = { s[0].get<float>(), s[1].get<float>(), s[2].get<float>() };
        }
    }

    // -----------------------------------------------------------------------
    // 3.2.2  AdPBRMaterialComponent
    // -----------------------------------------------------------------------
    static nlohmann::json SerializePBRMaterial(const AdComponent& comp) {
        const auto& matComp = static_cast<const AdPBRMaterialComponent&>(comp);

        // If we have serialization metadata (from a previous Load), use it as the base
        // and update only the live scalar material properties
        if (matComp.HasSerializationMeta()) {
            nlohmann::json result = matComp.GetSerializationMeta();

            // Update material scalar properties from live material objects
            const auto& meshMaterials = matComp.GetMeshMaterials();
            if (result.contains("materials") && !meshMaterials.empty()) {
                int matIdx = 0;
                for (const auto& [material, meshIndices] : meshMaterials) {
                    if (material && matIdx < (int)result["materials"].size()) {
                        auto& matJson = result["materials"][matIdx];
                        const auto& bcf = material->GetBaseColorFactor();
                        matJson["baseColorFactor"] = { bcf.x, bcf.y, bcf.z, bcf.w };
                        matJson["metallicFactor"] = material->GetMetallicFactor();
                        matJson["roughnessFactor"] = material->GetRoughnessFactor();
                        matJson["aoFactor"] = material->GetAoFactor();
                        matJson["emissiveFactor"] = material->GetEmissiveFactor();
                    }
                    matIdx++;
                }
            }
            return result;
        }

        // Fallback: no metadata, serialize from scratch (primitives only)
        nlohmann::json result;
        nlohmann::json materialsArr = nlohmann::json::array();
        nlohmann::json meshesArr = nlohmann::json::array();

        const auto& meshMaterials = matComp.GetMeshMaterials();
        int materialIdx = 0;
        for (const auto& [material, meshIndices] : meshMaterials) {
            nlohmann::json matJson;
            if (material) {
                const auto& bcf = material->GetBaseColorFactor();
                matJson["baseColorFactor"] = { bcf.x, bcf.y, bcf.z, bcf.w };
                matJson["metallicFactor"] = material->GetMetallicFactor();
                matJson["roughnessFactor"] = material->GetRoughnessFactor();
                matJson["aoFactor"] = material->GetAoFactor();
                matJson["emissiveFactor"] = material->GetEmissiveFactor();
                matJson["textures"] = {
                    { "baseColor", nullptr }, { "normal", nullptr },
                    { "metallicRoughness", nullptr }, { "ao", nullptr }, { "emissive", nullptr }
                };
            }
            materialsArr.push_back(matJson);

            for (uint32_t mi : meshIndices) {
                nlohmann::json meshJson;
                meshJson["type"] = "primitive";
                meshJson["shape"] = "cube";
                meshJson["materialIndex"] = materialIdx;
                meshesArr.push_back(meshJson);
            }
            materialIdx++;
        }

        result["materials"] = materialsArr;
        result["meshes"] = meshesArr;
        return result;
    }

    static void DeserializePBRMaterial(AdEntity* entity, const nlohmann::json& j, const nlohmann::json& resources) {
        auto& matComp = entity->HasComponent<AdPBRMaterialComponent>()
                          ? entity->GetComponent<AdPBRMaterialComponent>()
                          : entity->AddComponent<AdPBRMaterialComponent>();

        if (!j.contains("materials") || !j.contains("meshes")) {
            LOG_W("DeserializePBRMaterial: missing 'materials' or 'meshes' array");
            return;
        }

        // Store the original JSON + resolved resource keys as serialization metadata
        // so that Save can write back the model/texture references correctly
        {
            nlohmann::json meta = j;
            // Resolve resource keys to include the actual resource entries inline
            // so Save doesn't need the global resources object
            if (meta.contains("materials")) {
                for (auto& matJson : meta["materials"]) {
                    if (!matJson.contains("textures")) continue;
                    auto& textures = matJson["textures"];
                    auto resolveTexRes = [&](const char* name) {
                        if (!textures.contains(name) || textures[name].is_null()) return;
                        auto& texEntry = textures[name];
                        if (!texEntry.contains("resource") || texEntry["resource"].is_null()) return;
                        std::string resKey = texEntry["resource"].get<std::string>();
                        if (resources.contains(resKey)) {
                            texEntry["_resourceKey"] = resKey;
                            texEntry["_resourcePath"] = resources[resKey]["path"].get<std::string>();
                        }
                    };
                    resolveTexRes("baseColor");
                    resolveTexRes("normal");
                    resolveTexRes("metallicRoughness");
                    resolveTexRes("ao");
                    resolveTexRes("emissive");
                }
            }
            if (meta.contains("meshes")) {
                for (auto& meshJson : meta["meshes"]) {
                    if (!meshJson.contains("resource") || meshJson["resource"].is_null()) continue;
                    std::string resKey = meshJson["resource"].get<std::string>();
                    if (resources.contains(resKey)) {
                        meshJson["_resourceKey"] = resKey;
                        meshJson["_resourcePath"] = resources[resKey]["path"].get<std::string>();
                    }
                }
            }
            matComp.SetSerializationMeta(meta);
        }

        const auto& materialsArr = j["materials"];
        const auto& meshesArr = j["meshes"];

        // Create materials
        std::vector<AdPBRMaterial*> createdMaterials;
        // Shared sampler for all textures loaded here
        static auto sharedSampler = std::make_shared<AdSampler>();
        // Keep textures alive
        static std::vector<std::shared_ptr<AdTexture>> loadedTextures;

        for (const auto& matJson : materialsArr) {
            AdPBRMaterial* mat = AdMaterialFactory::GetInstance()->CreateMaterial<AdPBRMaterial>();

            if (matJson.contains("baseColorFactor")) {
                auto& bcf = matJson["baseColorFactor"];
                mat->SetBaseColorFactor({ bcf[0].get<float>(), bcf[1].get<float>(),
                                          bcf[2].get<float>(), bcf[3].get<float>() });
            }
            if (matJson.contains("metallicFactor"))  mat->SetMetallicFactor(matJson["metallicFactor"].get<float>());
            if (matJson.contains("roughnessFactor")) mat->SetRoughnessFactor(matJson["roughnessFactor"].get<float>());
            if (matJson.contains("aoFactor"))        mat->SetAoFactor(matJson["aoFactor"].get<float>());
            if (matJson.contains("emissiveFactor"))  mat->SetEmissiveFactor(matJson["emissiveFactor"].get<float>());

            // Load textures from resource references
            if (matJson.contains("textures")) {
                auto loadTex = [&](const char* name, uint32_t slot) {
                    if (!matJson["textures"].contains(name) || matJson["textures"][name].is_null()) return;
                    auto& texEntry = matJson["textures"][name];
                    if (!texEntry.contains("resource") || texEntry["resource"].is_null()) return;

                    std::string resKey = texEntry["resource"].get<std::string>();
                    if (!resources.contains(resKey)) {
                        LOG_W("DeserializePBRMaterial: resource key '{0}' not found in resources", resKey);
                        return;
                    }
                    std::string relPath = resources[resKey]["path"].get<std::string>();
                    std::string absPath = ToAbsolutePath(relPath);

                    auto tex = std::make_shared<AdTexture>(absPath);
                    mat->SetTextureView(slot, tex.get(), sharedSampler.get());
                    loadedTextures.push_back(tex);
                };
                loadTex("baseColor", PBR_MAT_BASE_COLOR);
                loadTex("normal", PBR_MAT_NORMAL);
                loadTex("metallicRoughness", PBR_MAT_METALLIC_ROUGHNESS);
                loadTex("ao", PBR_MAT_AO);
                loadTex("emissive", PBR_MAT_EMISSIVE);
            }

            createdMaterials.push_back(mat);
        }

        // Create meshes and bind to materials
        // Keep model resources and meshes alive
        static std::vector<std::shared_ptr<AdModelResource>> loadedModels;
        static std::vector<std::shared_ptr<AdMesh>> loadedMeshes;

        for (const auto& meshJson : meshesArr) {
            std::string meshType = meshJson.value("type", "primitive");
            int materialIndex = meshJson.value("materialIndex", 0);
            AdPBRMaterial* mat = (materialIndex >= 0 && materialIndex < (int)createdMaterials.size())
                                   ? createdMaterials[materialIndex] : nullptr;

            if (meshType == "model") {
                if (!meshJson.contains("resource") || meshJson["resource"].is_null()) {
                    LOG_W("DeserializePBRMaterial: model mesh missing 'resource' key");
                    continue;
                }
                std::string resKey = meshJson["resource"].get<std::string>();
                if (!resources.contains(resKey)) {
                    LOG_W("DeserializePBRMaterial: resource key '{0}' not found", resKey);
                    continue;
                }
                std::string relPath = resources[resKey]["path"].get<std::string>();
                std::string absPath = ToAbsolutePath(relPath);
                int meshIndex = meshJson.value("meshIndex", 0);

                auto model = std::make_shared<AdModelResource>(absPath);
                if (model->Load()) {
                    const auto& modelMeshes = model->GetMeshes();
                    if (meshIndex >= 0 && meshIndex < (int)modelMeshes.size()) {
                        auto mesh = std::make_shared<AdMesh>(modelMeshes[meshIndex].Vertices,
                                                              modelMeshes[meshIndex].Indices);
                        matComp.AddMesh(mesh.get(), mat);
                        loadedMeshes.push_back(mesh);
                    } else {
                        // Load all meshes from the model
                        for (size_t i = 0; i < modelMeshes.size(); i++) {
                            auto mesh = std::make_shared<AdMesh>(modelMeshes[i].Vertices,
                                                                  modelMeshes[i].Indices);
                            matComp.AddMesh(mesh.get(), mat);
                            loadedMeshes.push_back(mesh);
                        }
                    }
                    loadedModels.push_back(model);
                } else {
                    LOG_E("DeserializePBRMaterial: failed to load model '{0}'", absPath);
                }
            } else if (meshType == "primitive") {
                std::string shape = meshJson.value("shape", "cube");
                if (shape == "cube") {
                    std::vector<AdVertex> vertices;
                    std::vector<uint32_t> indices;
                    AdGeometryUtil::CreateCube(-0.3f, 0.3f, -0.3f, 0.3f, -0.3f, 0.3f, vertices, indices);
                    auto mesh = std::make_shared<AdMesh>(vertices, indices);
                    matComp.AddMesh(mesh.get(), mat);
                    loadedMeshes.push_back(mesh);
                } else {
                    LOG_W("DeserializePBRMaterial: unrecognized primitive shape '{0}'", shape);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // 3.2.3  AdDirectionalLightComponent
    // -----------------------------------------------------------------------
    static nlohmann::json SerializeDirectionalLight(const AdComponent& comp) {
        const auto& light = static_cast<const AdDirectionalLightComponent&>(comp);
        const auto& dir = light.GetDirection();
        const auto& col = light.GetColor();
        return {
            { "direction", { dir.x, dir.y, dir.z } },
            { "color",     { col.x, col.y, col.z } },
            { "intensity", light.GetIntensity() },
            { "enabled",   light.IsEnabled() }
        };
    }

    static void DeserializeDirectionalLight(AdEntity* entity, const nlohmann::json& j, const nlohmann::json& /*resources*/) {
        auto& light = entity->HasComponent<AdDirectionalLightComponent>()
                        ? entity->GetComponent<AdDirectionalLightComponent>()
                        : entity->AddComponent<AdDirectionalLightComponent>();
        if (j.contains("direction")) {
            auto& d = j["direction"];
            light.SetDirection({ d[0].get<float>(), d[1].get<float>(), d[2].get<float>() });
        }
        if (j.contains("color")) {
            auto& c = j["color"];
            light.SetColor({ c[0].get<float>(), c[1].get<float>(), c[2].get<float>() });
        }
        if (j.contains("intensity")) light.SetIntensity(j["intensity"].get<float>());
        if (j.contains("enabled"))   light.SetEnabled(j["enabled"].get<bool>());
    }

    // -----------------------------------------------------------------------
    // 3.2.4  AdPointLightComponent
    // -----------------------------------------------------------------------
    static nlohmann::json SerializePointLight(const AdComponent& comp) {
        const auto& light = static_cast<const AdPointLightComponent&>(comp);
        const auto& col = light.GetColor();
        return {
            { "color",     { col.x, col.y, col.z } },
            { "intensity", light.GetIntensity() },
            { "enabled",   light.IsEnabled() },
            { "range",     light.GetRange() },
            { "attenuation", {
                { "constant",  light.GetAttenuationConstant() },
                { "linear",    light.GetAttenuationLinear() },
                { "quadratic", light.GetAttenuationQuadratic() }
            }}
        };
    }

    static void DeserializePointLight(AdEntity* entity, const nlohmann::json& j, const nlohmann::json& /*resources*/) {
        auto& light = entity->HasComponent<AdPointLightComponent>()
                        ? entity->GetComponent<AdPointLightComponent>()
                        : entity->AddComponent<AdPointLightComponent>();
        if (j.contains("color")) {
            auto& c = j["color"];
            light.SetColor({ c[0].get<float>(), c[1].get<float>(), c[2].get<float>() });
        }
        if (j.contains("intensity")) light.SetIntensity(j["intensity"].get<float>());
        if (j.contains("enabled"))   light.SetEnabled(j["enabled"].get<bool>());
        if (j.contains("range"))     light.SetRange(j["range"].get<float>());
        if (j.contains("attenuation")) {
            auto& att = j["attenuation"];
            light.SetAttenuation(
                att.value("constant", 1.0f),
                att.value("linear", 0.14f),
                att.value("quadratic", 0.07f)
            );
        }
    }

    // -----------------------------------------------------------------------
    // 3.2.5  AdSpotLightComponent
    // -----------------------------------------------------------------------
    static nlohmann::json SerializeSpotLight(const AdComponent& comp) {
        const auto& light = static_cast<const AdSpotLightComponent&>(comp);
        const auto& col = light.GetColor();
        return {
            { "color",        { col.x, col.y, col.z } },
            { "intensity",    light.GetIntensity() },
            { "enabled",      light.IsEnabled() },
            { "range",        light.GetRange() },
            { "attenuation", {
                { "constant",  light.GetAttenuationConstant() },
                { "linear",    light.GetAttenuationLinear() },
                { "quadratic", light.GetAttenuationQuadratic() }
            }},
            { "innerCutoff",  light.GetSpotInnerCutoff() },
            { "outerCutoff",  light.GetSpotOuterCutoff() }
        };
    }

    static void DeserializeSpotLight(AdEntity* entity, const nlohmann::json& j, const nlohmann::json& /*resources*/) {
        auto& light = entity->HasComponent<AdSpotLightComponent>()
                        ? entity->GetComponent<AdSpotLightComponent>()
                        : entity->AddComponent<AdSpotLightComponent>();
        if (j.contains("color")) {
            auto& c = j["color"];
            light.SetColor({ c[0].get<float>(), c[1].get<float>(), c[2].get<float>() });
        }
        if (j.contains("intensity"))   light.SetIntensity(j["intensity"].get<float>());
        if (j.contains("enabled"))     light.SetEnabled(j["enabled"].get<bool>());
        if (j.contains("range"))       light.SetRange(j["range"].get<float>());
        if (j.contains("attenuation")) {
            auto& att = j["attenuation"];
            light.SetAttenuation(
                att.value("constant", 1.0f),
                att.value("linear", 0.14f),
                att.value("quadratic", 0.07f)
            );
        }
        if (j.contains("innerCutoff") && j.contains("outerCutoff")) {
            light.SetSpotCutoff(j["innerCutoff"].get<float>(), j["outerCutoff"].get<float>());
        }
    }

    // -----------------------------------------------------------------------
    // 3.2.6  AdFirstPersonCameraComponent
    // -----------------------------------------------------------------------
    static nlohmann::json SerializeFirstPersonCamera(const AdComponent& comp) {
        const auto& cam = static_cast<const AdFirstPersonCameraComponent&>(comp);
        return {
            { "fov",         cam.GetFov() },
            { "nearPlane",   cam.GetNearPlane() },
            { "farPlane",    cam.GetFarPlane() },
            { "yaw",         cam.GetYaw() },
            { "pitch",       cam.GetPitch() },
            { "sensitivity", cam.GetSensitivity() },
            { "moveSpeed",   cam.GetMoveSpeed() }
        };
    }

    static void DeserializeFirstPersonCamera(AdEntity* entity, const nlohmann::json& j, const nlohmann::json& /*resources*/) {
        auto& cam = entity->HasComponent<AdFirstPersonCameraComponent>()
                      ? entity->GetComponent<AdFirstPersonCameraComponent>()
                      : entity->AddComponent<AdFirstPersonCameraComponent>();
        if (j.contains("fov"))         cam.SetFov(j["fov"].get<float>());
        if (j.contains("nearPlane"))   cam.SetNearPlane(j["nearPlane"].get<float>());
        if (j.contains("farPlane"))    cam.SetFarPlane(j["farPlane"].get<float>());
        if (j.contains("yaw"))         cam.SetYaw(j["yaw"].get<float>());
        if (j.contains("pitch"))       cam.SetPitch(j["pitch"].get<float>());
        if (j.contains("sensitivity")) cam.SetSensitivity(j["sensitivity"].get<float>());
        if (j.contains("moveSpeed"))   cam.SetMoveSpeed(j["moveSpeed"].get<float>());
    }

    // -----------------------------------------------------------------------
    // 3.2.7  AdLookAtCameraComponent
    // -----------------------------------------------------------------------
    static nlohmann::json SerializeLookAtCamera(const AdComponent& comp) {
        const auto& cam = static_cast<const AdLookAtCameraComponent&>(comp);
        const auto& target = cam.GetTarget();
        return {
            { "fov",         cam.GetFov() },
            { "nearPlane",   cam.GetNearPlane() },
            { "farPlane",    cam.GetFarPlane() },
            { "radius",      cam.GetRadius() },
            { "target",      { target.x, target.y, target.z } },
            { "sensitivity", cam.GetSensitivity() }
        };
    }

    static void DeserializeLookAtCamera(AdEntity* entity, const nlohmann::json& j, const nlohmann::json& /*resources*/) {
        auto& cam = entity->HasComponent<AdLookAtCameraComponent>()
                      ? entity->GetComponent<AdLookAtCameraComponent>()
                      : entity->AddComponent<AdLookAtCameraComponent>();
        if (j.contains("fov"))         cam.SetFov(j["fov"].get<float>());
        if (j.contains("nearPlane"))   cam.SetNearPlane(j["nearPlane"].get<float>());
        if (j.contains("farPlane"))    cam.SetFarPlane(j["farPlane"].get<float>());
        if (j.contains("radius"))      cam.SetRadius(j["radius"].get<float>());
        if (j.contains("target")) {
            auto& t = j["target"];
            cam.SetTarget({ t[0].get<float>(), t[1].get<float>(), t[2].get<float>() });
        }
        if (j.contains("sensitivity")) cam.SetSensitivity(j["sensitivity"].get<float>());
    }

    // -----------------------------------------------------------------------
    // Helper: try-serialize template for entity component lookup
    // -----------------------------------------------------------------------
    template<typename T>
    static TrySerializeEntityFn MakeTrySerialize(SerializeFn serializeFn) {
        return [serializeFn](AdEntity* entity) -> nlohmann::json {
            if (!entity || !entity->HasComponent<T>()) {
                return nlohmann::json{};
            }
            return serializeFn(entity->GetComponent<T>());
        };
    }

    // -----------------------------------------------------------------------
    // RegisterAll
    // -----------------------------------------------------------------------
    void AdBuiltinComponentSerializers::RegisterAll() {
        auto* reg = AdComponentSerializerRegistry::GetInstance();

        reg->Register("AdTransformComponent",
                       SerializeTransform, DeserializeTransform,
                       MakeTrySerialize<AdTransformComponent>(SerializeTransform));

        reg->Register("AdPBRMaterialComponent",
                       SerializePBRMaterial, DeserializePBRMaterial,
                       MakeTrySerialize<AdPBRMaterialComponent>(SerializePBRMaterial));

        reg->Register("AdDirectionalLightComponent",
                       SerializeDirectionalLight, DeserializeDirectionalLight,
                       MakeTrySerialize<AdDirectionalLightComponent>(SerializeDirectionalLight));

        reg->Register("AdPointLightComponent",
                       SerializePointLight, DeserializePointLight,
                       MakeTrySerialize<AdPointLightComponent>(SerializePointLight));

        reg->Register("AdSpotLightComponent",
                       SerializeSpotLight, DeserializeSpotLight,
                       MakeTrySerialize<AdSpotLightComponent>(SerializeSpotLight));

        reg->Register("AdFirstPersonCameraComponent",
                       SerializeFirstPersonCamera, DeserializeFirstPersonCamera,
                       MakeTrySerialize<AdFirstPersonCameraComponent>(SerializeFirstPersonCamera));

        reg->Register("AdLookAtCameraComponent",
                       SerializeLookAtCamera, DeserializeLookAtCamera,
                       MakeTrySerialize<AdLookAtCameraComponent>(SerializeLookAtCamera));
    }

} // namespace WuDu
