#define NOMINMAX
#include "Gui/AdResourceBrowserPanel.h"
#include "imgui/imgui.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace WuDu {

	static std::string ToLower(const std::string& str) {
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return result;
	}

	static bool IsTextureExtension(const std::string& ext) {
		std::string lower = ToLower(ext);
		return lower == ".png" || lower == ".jpg" || lower == ".jpeg"
			|| lower == ".tga" || lower == ".bmp" || lower == ".hdr";
	}

	static bool IsModelExtension(const std::string& ext) {
		std::string lower = ToLower(ext);
		return lower == ".obj" || lower == ".fbx"
			|| lower == ".gltf" || lower == ".glb";
	}

	static void ScanDirectory(const std::filesystem::path& dirPath,
	                          ResourceEntry::Category category,
	                          bool (*extensionCheck)(const std::string&),
	                          std::vector<ResourceEntry>& outEntries) {
		if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
			if (!entry.is_regular_file()) continue;

			std::string ext = entry.path().extension().string();
			if (extensionCheck(ext)) {
				ResourceEntry re;
				re.fileName = entry.path().filename().string();
				re.fullPath = entry.path().string();
				re.category = category;
				outEntries.push_back(std::move(re));
			}
		}
	}

	void AdResourceBrowserPanel::ScanResources(const std::string& resourceRootPath) {
		mTextures.clear();
		mModels.clear();

		try {
			std::filesystem::path rootPath(resourceRootPath);

			ScanDirectory(rootPath / "Texture", ResourceEntry::Category::Texture,
				IsTextureExtension, mTextures);

			ScanDirectory(rootPath / "Model", ResourceEntry::Category::Model,
				IsModelExtension, mModels);
		} catch (...) {
			// Handle any filesystem errors gracefully
			mTextures.clear();
			mModels.clear();
		}
	}

	static bool ContainsCaseInsensitive(const std::string& str, const std::string& substr) {
		std::string lowerStr = ToLower(str);
		std::string lowerSub = ToLower(substr);
		return lowerStr.find(lowerSub) != std::string::npos;
	}

	void AdResourceBrowserPanel::OnImGui() {
		ImGui::Begin("Resource Browser");

		// Search filter input
		char filterBuf[256];
		strncpy(filterBuf, mSearchFilter.c_str(), sizeof(filterBuf));
		filterBuf[sizeof(filterBuf) - 1] = '\0';
		if (ImGui::InputText("Search##ResourceFilter", filterBuf, sizeof(filterBuf))) {
			mSearchFilter = filterBuf;
		}

		ImGui::Separator();

		// Filter entries based on search string
		std::vector<ResourceEntry> filteredTextures;
		std::vector<ResourceEntry> filteredModels;

		for (const auto& entry : mTextures) {
			if (mSearchFilter.empty() || ContainsCaseInsensitive(entry.fileName, mSearchFilter)) {
				filteredTextures.push_back(entry);
			}
		}
		for (const auto& entry : mModels) {
			if (mSearchFilter.empty() || ContainsCaseInsensitive(entry.fileName, mSearchFilter)) {
				filteredModels.push_back(entry);
			}
		}

		if (filteredTextures.empty() && filteredModels.empty()) {
			ImGui::Text("No resources found");
		} else {
			DrawResourceList("Textures", filteredTextures);
			DrawResourceList("Models", filteredModels);
		}

		ImGui::End();
	}

	void AdResourceBrowserPanel::DrawResourceList(const std::string& categoryName,
	                                              const std::vector<ResourceEntry>& entries) {
		if (entries.empty()) return;

		if (ImGui::CollapsingHeader(categoryName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			for (const auto& entry : entries) {
				bool isSelected = (mSelectedResource == entry.fullPath);

				const char* typeLabel = (entry.category == ResourceEntry::Category::Texture) ? "[TEX]" : "[MDL]";
				std::string displayText = std::string(typeLabel) + " " + entry.fileName;

				ImGui::PushID(entry.fullPath.c_str());
				if (ImGui::Selectable(displayText.c_str(), isSelected)) {
					mSelectedResource = entry.fullPath;
				}

				// Set up drag source for texture entries
				if (entry.category == ResourceEntry::Category::Texture) {
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						ImGui::SetDragDropPayload("TEXTURE_PATH", entry.fullPath.c_str(), entry.fullPath.size() + 1);
						ImGui::Text("%s", entry.fileName.c_str());
						ImGui::EndDragDropSource();
					}
				}

				// Set up drag source for model entries
				if (entry.category == ResourceEntry::Category::Model) {
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						ImGui::SetDragDropPayload("MODEL_PATH", entry.fullPath.c_str(), entry.fullPath.size() + 1);
						ImGui::Text("%s", entry.fileName.c_str());
						ImGui::EndDragDropSource();
					}
				}

				ImGui::PopID();
			}
		}
	}
}
