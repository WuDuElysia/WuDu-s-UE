#include "ECS/System/AdMaterialSystem.h"

#include "AdApplication.h"
#include "Render/AdRenderContext.h"
#include "Render/AdRenderTarget.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"

namespace WuDu {
	/**
 * @brief 获取应用程序实例指针
 *
 * 通过全局应用上下文获取当前运行的应用程序实例。
 *
 * @return AdApplication* 应用程序实例指针，若上下文无效则返回nullptr
 */
	AdApplication* AdMaterialSystem::GetApp() const {
		AdAppContext* appContext = AdApplication::GetAppContext();
		if (appContext) {
			return appContext->app;
		}
		return nullptr;
	}

	/**
	 * @brief 获取当前场景实例指针
	 *
	 * 通过全局应用上下文获取当前活动的场景实例。
	 *
	 * @return AdScene* 场景实例指针，若上下文无效则返回nullptr
	 */
	AdScene* AdMaterialSystem::GetScene() const {
		AdAppContext* appContext = AdApplication::GetAppContext();
		if (appContext) {
			return appContext->scene;
		}
		return nullptr;
	}

	/**
	 * @brief 获取 Vulkan 设备实例指针
	 *
	 * 通过全局应用上下文和渲染上下文获取 Vulkan 设备实例。
	 *
	 * @return AdVKDevice* Vulkan 设备实例指针，若上下文或渲染上下文无效则返回nullptr
	 */
	AdVKDevice* AdMaterialSystem::GetDevice() const {
		AdAppContext* appContext = AdApplication::GetAppContext();
		if (appContext) {
			if (appContext->renderCxt) {
				return appContext->renderCxt->GetDevice();
			}
		}
		return nullptr;
	}

	/**
	 * @brief 获取指定渲染目标的投影矩阵
	 *
	 * 根据渲染目标绑定的相机类型（LookAt 或 FirstPerson）获取对应的投影矩阵。
	 *
	 * @param renderTarget 渲染目标实例指针
	 * @return const glm::mat4 投影矩阵，若未找到有效相机组件则返回单位矩阵
	 */
	const glm::mat4 AdMaterialSystem::GetProjMat(AdRenderTarget* renderTarget) const {
		glm::mat4 projMat{ 1.f };
		AdEntity* camera = renderTarget->GetCamera();
		// 检查相机实体是否包含 LookAt 相机组件
		if (AdEntity::HasComponent<AdLookAtCameraComponent>(camera)) {
			auto& cameraComp = camera->GetComponent<AdLookAtCameraComponent>();
			projMat = cameraComp.GetProjMat();
		}
		// 若不包含 LookAt 组件，则检查是否为第一人称相机
		else if (AdEntity::HasComponent<AdFirstPersonCameraComponent>(camera)) {
			auto& cameraComp = camera->GetComponent<AdFirstPersonCameraComponent>();
			projMat = cameraComp.GetProjMat();
		}
		return projMat;
	}

	/**
	 * @brief 获取指定渲染目标的视图矩阵
	 *
	 * 根据渲染目标绑定的相机类型（LookAt 或 FirstPerson）获取对应的视图矩阵。
	 *
	 * @param renderTarget 渲染目标实例指针
	 * @return const glm::mat4 视图矩阵，若未找到有效相机组件则返回单位矩阵
	 */
	const glm::mat4 AdMaterialSystem::GetViewMat(AdRenderTarget* renderTarget) const {
		glm::mat4 viewMat{ 1.f };
		AdEntity* camera = renderTarget->GetCamera();
		// 检查相机实体是否包含 LookAt 相机组件
		if (AdEntity::HasComponent<AdLookAtCameraComponent>(camera)) {
			auto& cameraComp = camera->GetComponent<AdLookAtCameraComponent>();
			viewMat = cameraComp.GetViewMat();
		}
		// 若不包含 LookAt 组件，则检查是否为第一人称相机
		else if (AdEntity::HasComponent<AdFirstPersonCameraComponent>(camera)) {
			auto& cameraComp = camera->GetComponent<AdFirstPersonCameraComponent>();
			viewMat = cameraComp.GetViewMat();
		}
		return viewMat;
	}
}