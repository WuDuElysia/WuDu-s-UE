#pragma once
#include <string>
#include <vector>

namespace WuDu {

	struct ResourceEntry {
		std::string fileName;
		std::string fullPath;
		enum class Category { Texture, Model } category;
	};

	class AdResourceBrowserPanel {
	public:
		void OnImGui();
		void ScanResources(const std::string& resourceRootPath);

	private:
		void DrawResourceList(const std::string& categoryName,
		                      const std::vector<ResourceEntry>& entries);

		std::vector<ResourceEntry> mTextures;
		std::vector<ResourceEntry> mModels;
		std::string mSearchFilter;
		std::string mSelectedResource;
	};
}
