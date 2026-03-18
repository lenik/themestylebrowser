#ifndef TSB_LIBRARY_MODEL_HPP
#define TSB_LIBRARY_MODEL_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

namespace tsb {

/**
 * Theme style entry from .themestyles (e.g. "flex/regular", "pixel").
 */
struct ThemeStyle {
    std::string id;  // as in file: "flex/regular"
    std::string var; // for code: replace('/', '_') e.g. "flex_regular"
};

/**
 * Single image file under a themestyle.
 */
struct ImageEntry {
    std::string themestyle_id;
    std::string rel_path;   // path relative to package root
    std::string full_path;  // absolute path
    std::string ext;
    int64_t size{0};
    time_t mtime{0};
};

/**
 * One logical "item": same base name across themestyles (e.g. "camera" -> camera.png in style1, camera.jpg in style2).
 */
struct FileItem {
    std::string name;       // base name without extension (e.g. "camera")
    int count{0};           // number of themestyles that define this item
    std::vector<std::string> formats; // e.g. ["PNG","JPG"]
    std::map<std::string, int> format_counts; // format -> count for display
    int64_t total_bytes{0};
    std::vector<ImageEntry> images;   // one per themestyle that has this file
};

/**
 * Package path is the path under a theme/style (e.g. "computer", "computer/travel").
 * Stored as vector of segments for tree building.
 */
using PackagePath = std::vector<std::string>;

/**
 * Library model: .themestyles + package tree + file index per package.
 */
class LibraryModel {
public:
    LibraryModel() = default;

    bool load(const std::string& library_root);
    void clear();

    const std::string& getLibraryRoot() const { return m_root; }
    const std::vector<ThemeStyle>& getThemeStyles() const { return m_theme_styles; }
    const std::set<PackagePath>& getPackagePaths() const { return m_package_paths; }

    /** Get theme/style IDs that have this package directory (for current package column visibility). */
    std::vector<std::string> getThemeStylesForPackage(const PackagePath& pkg) const;

    /** Get file items for a package path. */
    std::vector<FileItem> getFileItems(const PackagePath& pkg) const;

    /** File count (total image files) and item count (distinct base names) for a package. */
    int getPackageFileCount(const PackagePath& pkg) const;
    int getPackageItemCount(const PackagePath& pkg) const;
    /** Total size in bytes for a package (sum of all file sizes). */
    int64_t getPackageTotalSize(const PackagePath& pkg) const;

    /** Resolve full path for an image. */
    std::string getThemeStyleDir(const std::string& themestyle_id) const;

private:
    bool parseThemestylesFile(const std::string& path);
    void scanPackageTree();
    void scanPackageFiles(const PackagePath& pkg);

    std::string m_root;
    std::vector<ThemeStyle> m_theme_styles;
    std::set<PackagePath> m_package_paths;
    std::map<PackagePath, std::vector<FileItem>> m_package_files;
};

} // namespace tsb

#endif
