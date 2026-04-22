#define NOMINMAX
#include "ECS/AdEntity.h"
#include "Gui/AdEditorContext.h"

namespace WuDu {
	void AdEditorContext::ValidateSelection() {
		if (selectedEntity && !selectedEntity->IsValid()) {
			selectedEntity = nullptr;
		}
	}
}
