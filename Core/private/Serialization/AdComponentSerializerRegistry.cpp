#define NOMINMAX
#include "Serialization/AdComponentSerializerRegistry.h"
#include "AdLog.h"

namespace WuDu {

    AdComponentSerializerRegistry* AdComponentSerializerRegistry::GetInstance() {
        static AdComponentSerializerRegistry instance;
        return &instance;
    }

    void AdComponentSerializerRegistry::Register(const std::string& typeName,
                                                  SerializeFn serialize,
                                                  DeserializeFn deserialize,
                                                  TrySerializeEntityFn trySerializeEntity) {
        auto it = mSerializers.find(typeName);
        if (it != mSerializers.end()) {
            LOG_W("AdComponentSerializerRegistry: Overwriting serializer for '{0}'", typeName);
        } else {
            mRegisteredTypes.push_back(typeName);
        }
        mSerializers[typeName] = { std::move(serialize), std::move(deserialize), std::move(trySerializeEntity) };
    }

    nlohmann::json AdComponentSerializerRegistry::Serialize(const std::string& typeName,
                                                             const AdComponent& component) const {
        auto it = mSerializers.find(typeName);
        if (it == mSerializers.end()) {
            LOG_W("AdComponentSerializerRegistry: No serializer registered for '{0}'", typeName);
            return nlohmann::json{};
        }
        return it->second.serialize(component);
    }

    void AdComponentSerializerRegistry::Deserialize(const std::string& typeName,
                                                     AdEntity* entity,
                                                     const nlohmann::json& componentJson,
                                                     const nlohmann::json& resources) const {
        auto it = mSerializers.find(typeName);
        if (it == mSerializers.end()) {
            LOG_W("AdComponentSerializerRegistry: No deserializer registered for '{0}'", typeName);
            return;
        }
        it->second.deserialize(entity, componentJson, resources);
    }

    const std::vector<std::string>& AdComponentSerializerRegistry::GetRegisteredTypes() const {
        return mRegisteredTypes;
    }

    bool AdComponentSerializerRegistry::HasSerializer(const std::string& typeName) const {
        return mSerializers.find(typeName) != mSerializers.end();
    }

    nlohmann::json AdComponentSerializerRegistry::TrySerializeEntity(const std::string& typeName,
                                                                      AdEntity* entity) const {
        auto it = mSerializers.find(typeName);
        if (it == mSerializers.end() || !it->second.trySerializeEntity) {
            return nlohmann::json{};
        }
        return it->second.trySerializeEntity(entity);
    }

} // namespace WuDu
