// AdSceneEditor.h
#pragma once
#define NOMINMAX
#include "AdEngine.h"
#include "ECS/System/AdBaseMaterialSystem.h"
#include "imgui/imgui.h"
#include "ECS/AdEntity.h"
#include "ECS/Component/AdTransformComponent.h"
#include "ECS/Component/Material/AdUnlitMaterialComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "ECS/AdScene.h"

namespace WuDu {
    class AdSceneEditor {
    public:
        AdSceneEditor();
        ~AdSceneEditor() = default;

        // 设置编辑器所需资源
        void SetResources(
            AdScene* scene,
            AdEntity* activeCamera,
            AdMesh* cubeMesh,
            AdUnlitMaterial* defaultMaterial
        );

        // 添加场景编辑器UI
        void AddSceneEditor();
        
        // 显示场景层级视图
        void ShowSceneHierarchy();
        // 显示变换编辑器
        void ShowTransformEditor();
        // 处理场景视口
        void HandleSceneViewport();
        
        // 获取选中的实体
        AdEntity* GetSelectedEntity() const { return mSelectedEntity; }
        // 选择实体
        void SelectEntity(AdEntity* entity);
        // 在鼠标位置选择实体
        void SelectEntityAtMousePosition(float mouseX, float mouseY);
        // 处理变换拖动
        void HandleTransformDrag(const ImVec2& currentMousePos);

    private:
        // 场景编辑模式
        enum class TransformMode { Translate, Rotate, Scale };
        TransformMode mCurrentTransformMode = TransformMode::Translate;

        // 场景资源
        AdScene* mScene = nullptr;
        AdEntity* mActiveCamera = nullptr;
        AdMesh* mCubeMesh = nullptr;
        AdUnlitMaterial* mDefaultMaterial = nullptr;

        // 编辑状态
        AdEntity* mSelectedEntity = nullptr;
        bool mIsDraggingInViewport = false;
        glm::vec2 mLastMousePos;
        glm::uvec2 mViewportSize = { 800, 600 };

        // 射线检测辅助函数
        float RayIntersectsCube(const glm::vec3& rayOrigin,
                            const glm::vec3& rayDirection,
                            const AdTransformComponent& transform);
    };
}