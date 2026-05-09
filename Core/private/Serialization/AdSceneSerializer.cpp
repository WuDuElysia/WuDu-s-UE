#define NOMINMAX
#include "Serialization/AdSceneSerializer.h"
#include "Serialization/AdComponentSerializerRegistry.h"
#include "ECS/AdScene.h"
#include "ECS/AdEntity.h"
#include "ECS/AdNode.h"
#include "AdFileUtil.h"
#include "AdLog.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace WuDu {

    // -----------------------------------------------------------------------
    // Helper: strip resource root prefix to get a relative path
    // -----------------------------------------------------------------------
    static std::string ToRelativePath(const std::string& absPath) {
        std::string root = AD_RES_ROOT_DIR;
        // Normalize separators for comparison
        std::string normalizedAbs = absPath;
        std::string normalizedRoot = root;
        std::replace(normalizedAbs.begin(), normalizedAbs.end(), '\\', '/');
        std::replace(normalizedRoot.begin(), normalizedRoot.end(), '\\', '/');
        if (normalizedAbs.find(normalizedRoot) == 0) {
            return normalizedAbs.substr(normalizedRoot.size());
        }
        return absPath;
    }

    // -----------------------------------------------------------------------
    // Helper: collect resource references from serialized component JSON
    // -----------------------------------------------------------------------
    static void CollectResources(nlohmann::json& componentJson,
                                 const std::string& typeName,
                                 nlohmann::json& resources,
                                 uint32_t& resourceCounter) {
        if (typeName != "AdPBRMaterialComponent") {
            return;
        }

        // Collect texture resources from materials (using _resourcePath metadata from Load)
        if (componentJson.contains("materials")) {
            for (auto& matJson : componentJson["materials"]) {
                if (!matJson.contains("textures")) continue;
                auto& textures = matJson["textures"];

                auto collectTex = [&](const char* slotName) {
                    if (!textures.contains(slotName) || textures[slotName].is_null()) return;
                    auto& texEntry = textures[slotName];

                    // Check for metadata from Load
                    std::string relPath;
                    std::string preferredKey;
                    if (texEntry.contains("_resourcePath")) {
                        relPath = texEntry["_resourcePath"].get<std::string>();
                        if (texEntry.contains("_resourceKey")) {
                            preferredKey = texEntry["_resourceKey"].get<std::string>();
                        }
                    } else if (texEntry.contains("resource") && !texEntry["resource"].is_null()) {
                        // Fallback: resource field might be a path string
                        std::string path = texEntry["resource"].get<std::string>();
                        if (!path.empty()) relPath = ToRelativePath(path);
                    }

                    if (relPath.empty()) return;

                    // Check if this resource is already registered
                    for (auto& [key, val] : resources.items()) {
                        if (val.contains("path") && val["path"].get<std::string>() == relPath) {
                            texEntry["resource"] = key;
                            // Clean up metadata fields
                            texEntry.erase("_resourcePath");
                            texEntry.erase("_resourceKey");
                            return;
                        }
                    }
                    // Create new resource entry
                    std::string resKey = preferredKey.empty()
                        ? ("tex_" + std::to_string(resourceCounter++))
                        : preferredKey;
                    resources[resKey] = {
                        { "type", "texture" },
                        { "path", relPath }
                    };
                    texEntry["resource"] = resKey;
                    texEntry.erase("_resourcePath");
                    texEntry.erase("_resourceKey");
                };

                collectTex("baseColor");
                collectTex("normal");
                collectTex("metallicRoughness");
                collectTex("ao");
                collectTex("emissive");
            }
        }

        // Collect model resources from meshes (using _resourcePath metadata from Load)
        if (componentJson.contains("meshes")) {
            for (auto& meshJson : componentJson["meshes"]) {
                std::string relPath;
                std::string preferredKey;

                if (meshJson.contains("_resourcePath")) {
                    relPath = meshJson["_resourcePath"].get<std::string>();
                    if (meshJson.contains("_resourceKey")) {
                        preferredKey = meshJson["_resourceKey"].get<std::string>();
                    }
                } else if (meshJson.contains("resource") && !meshJson["resource"].is_null()) {
                    std::string path = meshJson["resource"].get<std::string>();
                    if (!path.empty()) relPath = ToRelativePath(path);
                }

                if (relPath.empty()) continue;

                // Check if already registered
                bool found = false;
                for (auto& [key, val] : resources.items()) {
                    if (val.contains("path") && val["path"].get<std::string>() == relPath) {
                        meshJson["resource"] = key;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::string resKey = preferredKey.empty()
                        ? ("model_" + std::to_string(resourceCounter++))
                        : preferredKey;
                    resources[resKey] = {
                        { "type", "model" },
                        { "path", relPath }
                    };
                    meshJson["resource"] = resKey;
                }
                // Clean up metadata fields
                meshJson.erase("_resourcePath");
                meshJson.erase("_resourceKey");
            }
        }
    }

    // -----------------------------------------------------------------------
    // Save
    // -----------------------------------------------------------------------
    bool AdSceneSerializer::Save(const AdScene* scene, const std::string& filePath) {
        if (!scene) {
            LOG_E("AdSceneSerializer::Save: scene is null");
            return false;
        }

        auto* registry = AdComponentSerializerRegistry::GetInstance();
        const auto& registeredTypes = registry->GetRegisteredTypes();

        nlohmann::json root;
        root["version"] = 1;

        nlohmann::json resources = nlohmann::json::object();
        nlohmann::json entitiesArr = nlohmann::json::array();
        uint32_t resourceCounter = 0;

        // Iterate all entities
        const auto& entities = scene->GetEntities();
        for (const auto& [enttEntity, entityPtr] : entities) {
            AdEntity* entity = entityPtr.get();
            if (!entity || !entity->IsValid()) continue;

            nlohmann::json entityJson;
            entityJson["name"] = entity->GetName();
            entityJson["uuid"] = static_cast<uint64_t>(entity->GetId());

            // Parent UUID
            AdNode* parent = entity->GetParent();
            if (parent && parent != scene->GetRootNode()) {
                entityJson["parent"] = static_cast<uint64_t>(parent->GetId());
            } else {
                entityJson["parent"] = nullptr;
            }

            // Serialize components
            nlohmann::json componentsJson = nlohmann::json::object();
            for (const auto& typeName : registeredTypes) {
                nlohmann::json compJson = registry->TrySerializeEntity(typeName, entity);
                if (!compJson.is_null() && !compJson.empty()) {
                    // Collect resource references and update paths
                    CollectResources(compJson, typeName, resources, resourceCounter);
                    componentsJson[typeName] = compJson;
                }
            }
            entityJson["components"] = componentsJson;

            entitiesArr.push_back(entityJson);
        }

        root["resources"] = resources;
        root["entities"] = entitiesArr;

        // Write to file with 2-space indentation
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            LOG_E("AdSceneSerializer::Save: failed to open file for writing: {0}", filePath);
            return false;
        }

        try {
            outFile << root.dump(2);
            outFile.close();
            if (outFile.fail()) {
                LOG_E("AdSceneSerializer::Save: write error for file: {0}", filePath);
                return false;
            }
        } catch (const std::exception& e) {
            LOG_E("AdSceneSerializer::Save: exception writing file: {0}", e.what());
            return false;
        }

        LOG_I("AdSceneSerializer::Save: saved {0} entities, {1} resources to {2}",
              entitiesArr.size(), resources.size(), filePath);
        return true;
    }

    // -----------------------------------------------------------------------
    // Load
    // -----------------------------------------------------------------------
    bool AdSceneSerializer::Load(AdScene* scene, const std::string& filePath,
                                  uint32_t* outEntityCount,
                                  uint32_t* outResourceCount) {
        if (!scene) {
            LOG_E("AdSceneSerializer::Load: scene is null");
            if (outEntityCount) *outEntityCount = 0;
            if (outResourceCount) *outResourceCount = 0;
            return false;
        }

        // --- 5.1.1 / 5.1.6: Check file existence ---
        if (!std::filesystem::exists(filePath)) {
            LOG_E("AdSceneSerializer::Load: file not found: {0}", filePath);
            if (outEntityCount) *outEntityCount = 0;
            if (outResourceCount) *outResourceCount = 0;
            return false;
        }

        // Read file contents
        std::ifstream inFile(filePath);
        if (!inFile.is_open()) {
            LOG_E("AdSceneSerializer::Load: failed to open file: {0}", filePath);
            if (outEntityCount) *outEntityCount = 0;
            if (outResourceCount) *outResourceCount = 0;
            return false;
        }

        std::string fileContent((std::istreambuf_iterator<char>(inFile)),
                                 std::istreambuf_iterator<char>());
        inFile.close();

        // Parse JSON, catch malformed input
        nlohmann::json root;
        try {
            root = nlohmann::json::parse(fileContent);
        } catch (const nlohmann::json::parse_error& e) {
            LOG_E("AdSceneSerializer::Load: malformed JSON in {0}: {1}", filePath, e.what());
            if (outEntityCount) *outEntityCount = 0;
            if (outResourceCount) *outResourceCount = 0;
            return false;
        }

        // Validate version field
        if (!root.contains("version")) {
            LOG_E("AdSceneSerializer::Load: missing 'version' field in {0}", filePath);
            if (outEntityCount) *outEntityCount = 0;
            if (outResourceCount) *outResourceCount = 0;
            return false;
        }
        int version = root["version"].get<int>();
        if (version != 1) {
            LOG_E("AdSceneSerializer::Load: unsupported version {0} in {1}", version, filePath);
            if (outEntityCount) *outEntityCount = 0;
            if (outResourceCount) *outResourceCount = 0;
            return false;
        }

        // --- 5.1.2: First pass - clear scene and create entities with UUIDs ---
        scene->DestroyAllEntity();
        // Also clear root node children since DestroyAllEntity only clears registry/map
        AdNode* rootNode = scene->GetRootNode();
        if (rootNode) {
            while (rootNode->HasChildren()) {
                const auto& children = rootNode->GetChildren();
                rootNode->RemoveChild(children.back());
            }
        }

        nlohmann::json resources = root.value("resources", nlohmann::json::object());
        nlohmann::json entitiesArr = root.value("entities", nlohmann::json::array());

        // Build a UUID-to-entity map for hierarchy reconstruction
        std::unordered_map<uint64_t, AdEntity*> uuidToEntity;

        for (const auto& entityJson : entitiesArr) {
            uint64_t uuid = entityJson.value("uuid", (uint64_t)0);
            std::string name = entityJson.value("name", std::string("Entity"));

            AdEntity* entity = scene->CreateEntityWithUUID(AdUUID(static_cast<uint32_t>(uuid)), name);
            if (entity) {
                uuidToEntity[uuid] = entity;
            }
        }

        // --- 5.1.3: Second pass - re-establish parent-child hierarchy ---
        for (const auto& entityJson : entitiesArr) {
            uint64_t uuid = entityJson.value("uuid", (uint64_t)0);
            auto entityIt = uuidToEntity.find(uuid);
            if (entityIt == uuidToEntity.end()) continue;

            AdEntity* entity = entityIt->second;

            if (entityJson.contains("parent") && !entityJson["parent"].is_null()) {
                uint64_t parentUuid = entityJson["parent"].get<uint64_t>();
                auto parentIt = uuidToEntity.find(parentUuid);
                if (parentIt != uuidToEntity.end()) {
                    // SetParent calls AddChild which handles removing from old parent
                    entity->SetParent(parentIt->second);
                } else {
                    LOG_W("AdSceneSerializer::Load: parent UUID {0} not found for entity '{1}'",
                          parentUuid, entity->GetName());
                }
            }
        }

        // --- 5.1.4 / 5.1.5: Third pass - deserialize components ---
        auto* registry = AdComponentSerializerRegistry::GetInstance();

        for (const auto& entityJson : entitiesArr) {
            uint64_t uuid = entityJson.value("uuid", (uint64_t)0);
            auto entityIt = uuidToEntity.find(uuid);
            if (entityIt == uuidToEntity.end()) continue;

            AdEntity* entity = entityIt->second;

            if (!entityJson.contains("components") || !entityJson["components"].is_object()) continue;

            const auto& componentsJson = entityJson["components"];
            for (auto it = componentsJson.begin(); it != componentsJson.end(); ++it) {
                const std::string& typeName = it.key();
                const nlohmann::json& componentJson = it.value();

                if (registry->HasSerializer(typeName)) {
                    registry->Deserialize(typeName, entity, componentJson, resources);
                } else {
                    LOG_W("AdSceneSerializer::Load: no deserializer for component type '{0}'", typeName);
                }
            }
        }

        // --- 5.1.7: Report counts ---
        uint32_t entityCount = static_cast<uint32_t>(uuidToEntity.size());
        uint32_t resourceCount = static_cast<uint32_t>(resources.size());

        if (outEntityCount) *outEntityCount = entityCount;
        if (outResourceCount) *outResourceCount = resourceCount;

        LOG_I("AdSceneSerializer::Load: loaded {0} entities, {1} resources from {2}",
              entityCount, resourceCount, filePath);
        return true;
    }

} // namespace WuDu
