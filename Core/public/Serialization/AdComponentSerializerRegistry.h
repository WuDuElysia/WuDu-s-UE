#ifndef AD_COMPONENT_SERIALIZER_REGISTRY_H
#define AD_COMPONENT_SERIALIZER_REGISTRY_H

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "nlohmann/json.hpp"

namespace WuDu {

    class AdComponent;
    class AdEntity;

    using SerializeFn = std::function<nlohmann::json(const AdComponent&)>;
    using DeserializeFn = std::function<void(AdEntity*, const nlohmann::json&, const nlohmann::json& resources)>;
    // TrySerializeEntityFn: given an entity, check if it has the component and serialize it.
    // Returns empty json object if the entity does not have the component.
    using TrySerializeEntityFn = std::function<nlohmann::json(AdEntity*)>;

    class AdComponentSerializerRegistry {
    public:
        static AdComponentSerializerRegistry* GetInstance();

        void Register(const std::string& typeName, SerializeFn serialize, DeserializeFn deserialize,
                      TrySerializeEntityFn trySerializeEntity = nullptr);
        nlohmann::json Serialize(const std::string& typeName, const AdComponent& component) const;
        void Deserialize(const std::string& typeName, AdEntity* entity,
                         const nlohmann::json& componentJson, const nlohmann::json& resources) const;

        // Try to serialize a component from an entity by type name.
        // Returns empty json if the entity doesn't have the component or no trySerialize registered.
        nlohmann::json TrySerializeEntity(const std::string& typeName, AdEntity* entity) const;

        const std::vector<std::string>& GetRegisteredTypes() const;
        bool HasSerializer(const std::string& typeName) const;

    private:
        AdComponentSerializerRegistry() = default;

        struct SerializerEntry {
            SerializeFn serialize;
            DeserializeFn deserialize;
            TrySerializeEntityFn trySerializeEntity;
        };

        std::unordered_map<std::string, SerializerEntry> mSerializers;
        std::vector<std::string> mRegisteredTypes;
    };

} // namespace WuDu

#endif // AD_COMPONENT_SERIALIZER_REGISTRY_H
