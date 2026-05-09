#ifndef AD_SCENE_SERIALIZER_H
#define AD_SCENE_SERIALIZER_H

#include <string>
#include <cstdint>

namespace WuDu {

    class AdScene;

    class AdSceneSerializer {
    public:
        // Save the scene to a JSON file. Returns true on success.
        static bool Save(const AdScene* scene, const std::string& filePath);

        // Load a scene from a JSON file. Clears existing scene content first.
        // Returns true on success, populates outEntityCount/outResourceCount.
        static bool Load(AdScene* scene, const std::string& filePath,
                         uint32_t* outEntityCount = nullptr,
                         uint32_t* outResourceCount = nullptr);
    };

} // namespace WuDu

#endif // AD_SCENE_SERIALIZER_H
