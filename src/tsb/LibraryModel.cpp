#include "tsb/LibraryModel.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

namespace fs = std::filesystem;

namespace tsb {

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
}

static std::string themeStyleToVar(const std::string& id) {
    std::string v = id;
    std::replace(v.begin(), v.end(), '/', '_');
    return v;
}

bool LibraryModel::parseThemestylesFile(const std::string& path) {
    m_theme_styles.clear();
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        ThemeStyle ts;
        ts.id = line;
        ts.var = themeStyleToVar(line);
        m_theme_styles.push_back(std::move(ts));
    }
    return true;
}

static fs::path themeStyleToPath(const std::string& root, const std::string& themestyle_id) {
    fs::path dir(root);
    size_t pos = 0;
    for (;;) {
        size_t next = themestyle_id.find('/', pos);
        if (next == std::string::npos) {
            if (pos < themestyle_id.size())
                dir /= themestyle_id.substr(pos);
            break;
        }
        dir /= themestyle_id.substr(pos, next - pos);
        pos = next + 1;
    }
    return dir;
}

void LibraryModel::scanPackageTree() {
    m_package_paths.clear();
    m_package_files.clear();

    for (const auto& ts : m_theme_styles) {
        fs::path dir = themeStyleToPath(m_root, ts.id);
        if (!fs::exists(dir) || !fs::is_directory(dir)) continue;

        m_package_paths.insert({}); // root package
        std::function<void(const fs::path&, PackagePath)> scan;
        scan = [this, &scan](const fs::path& d, PackagePath prefix) {
            for (const auto& e : fs::directory_iterator(d)) {
                if (!e.is_directory()) continue;
                std::string name = e.path().filename().string();
                if (name.empty() || name[0] == '.') continue;
                PackagePath p = prefix;
                p.push_back(name);
                m_package_paths.insert(p);
                scan(e.path(), p);
            }
        };
        scan(dir, {});
    }
}

void LibraryModel::scanPackageFiles(const PackagePath& pkg) {
    std::map<std::string, FileItem> by_name; // base name -> item

    for (const auto& ts : m_theme_styles) {
        fs::path dir = themeStyleToPath(m_root, ts.id);
        for (const auto& seg : pkg) dir /= seg;

        if (!fs::exists(dir) || !fs::is_directory(dir)) continue;

        for (const auto& e : fs::directory_iterator(dir)) {
            if (!e.is_regular_file()) continue;
            std::string fname = e.path().filename().string();
            std::string ext;
            size_t dot = fname.rfind('.');
            std::string base = (dot != std::string::npos) ? fname.substr(0, dot) : fname;
            if (dot != std::string::npos && dot + 1 < fname.size())
                ext = fname.substr(dot + 1);

            ImageEntry ie;
            ie.themestyle_id = ts.id;
            ie.rel_path = fname;  // path relative to package root (this dir)
            ie.full_path = e.path().string();
            ie.ext = ext;
            try {
                auto st = fs::status(e.path());
                if (fs::is_regular_file(st)) {
                    ie.size = fs::file_size(e.path());
                    // mtime: use 0 here; dialogs can use wxFileName for display
                }
            } catch (...) {}

            FileItem& item = by_name[base];
            item.name = base;
            item.count = 0;
            item.images.push_back(std::move(ie));
        }
    }

    std::vector<FileItem> items;
    for (auto& kv : by_name) {
        FileItem& it = kv.second;
        it.count = static_cast<int>(it.images.size());
        it.total_bytes = 0;
        std::map<std::string, int> format_counts;
        for (const auto& im : it.images) {
            it.total_bytes += im.size;
            if (!im.ext.empty()) {
                std::string u = im.ext;
                for (char& c : u) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                format_counts[u]++;
            }
        }
        for (const auto& kv : format_counts) {
            it.formats.push_back(kv.first);
            it.format_counts[kv.first] = kv.second;
        }
        items.push_back(std::move(it));
    }
    m_package_files[pkg] = std::move(items);
}

bool LibraryModel::load(const std::string& library_root) {
    clear();
    fs::path root(library_root);
    if (!fs::exists(root) || !fs::is_directory(root)) return false;
    m_root = fs::absolute(root).string();
    if (m_root.back() == '/') m_root.pop_back();

    std::string dot_path = m_root + "/.themestyles";
    if (!parseThemestylesFile(dot_path)) return false;
    scanPackageTree();

    for (const auto& pkg : m_package_paths)
        scanPackageFiles(pkg);

    return true;
}

void LibraryModel::clear() {
    m_root.clear();
    m_theme_styles.clear();
    m_package_paths.clear();
    m_package_files.clear();
}

std::vector<std::string> LibraryModel::getThemeStylesForPackage(const PackagePath& pkg) const {
    std::vector<std::string> out;
    for (const auto& ts : m_theme_styles) {
        fs::path dir = themeStyleToPath(m_root, ts.id);
        for (const auto& seg : pkg) dir /= seg;
        if (fs::exists(dir) && fs::is_directory(dir))
            out.push_back(ts.id);
    }
    return out;
}

std::vector<FileItem> LibraryModel::getFileItems(const PackagePath& pkg) const {
    auto it = m_package_files.find(pkg);
    if (it == m_package_files.end()) return {};
    return it->second;
}

std::string LibraryModel::getThemeStyleDir(const std::string& themestyle_id) const {
    return themeStyleToPath(m_root, themestyle_id).string();
}

int LibraryModel::getPackageFileCount(const PackagePath& pkg) const {
    auto it = m_package_files.find(pkg);
    if (it == m_package_files.end()) return 0;
    int n = 0;
    for (const auto& item : it->second) n += static_cast<int>(item.images.size());
    return n;
}

int LibraryModel::getPackageItemCount(const PackagePath& pkg) const {
    auto it = m_package_files.find(pkg);
    if (it == m_package_files.end()) return 0;
    return static_cast<int>(it->second.size());
}

int64_t LibraryModel::getPackageTotalSize(const PackagePath& pkg) const {
    auto it = m_package_files.find(pkg);
    if (it == m_package_files.end()) return 0;
    int64_t s = 0;
    for (const auto& item : it->second) s += item.total_bytes;
    return s;
}

} // namespace tsb
