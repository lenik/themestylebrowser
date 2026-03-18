#ifndef TSB_TSB_FRAME_HPP
#define TSB_TSB_FRAME_HPP

#include "LibraryModel.hpp"
#include "ui/arch/UIState.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>

#include <map>
#include <memory>
#include <vector>

namespace tsb {

class TSBCore : public UIFragment {
  public:
    enum SortMethod {
        SORT_NAME = 0,
        SORT_COUNT = 1,
        SORT_SIZE = 2,
    };

    enum Column {
        COL_NAME = 0,
        COL_COUNT = 1,
        COL_SIZE = 2,
        COL_STYLE_BASE = 3,
    };

    TSBCore();

    void openLibrary(const wxString& path);
    void refreshLibrary();
    wxBitmap loadThumbnail(const std::string& path, int size = 32);

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

  private:
    void onOpenLibrary(PerformContext*);
    void onExit(PerformContext*);

    void onTreeSelChanged(wxTreeEvent&);
    void onTreeItemActivated(wxTreeEvent&);
    void onGridCellLeftDClick(wxGridEvent&);
    void onGridCellRightClick(wxGridEvent&);
    void onGridLabelRightClick(wxGridEvent&);
    void onGridLabelLeftClick(wxGridEvent&);
    void onColVisibilityMenu(wxCommandEvent&);
    void onDeleteImage(wxCommandEvent&);
    void onDeleteItem(wxCommandEvent&);
    void onRenameItem(wxCommandEvent&);

    void buildPackageTree();
    std::vector<PackagePath> getSortedDirectChildren(const PackagePath& parent_prefix) const;
    void selectPackagePath(const std::vector<std::string>& pkg);
    void refreshFileList();
    void refreshThemeStyleColumns();
    static wxString formatSize(int64_t bytes);
    int getStyleColumnIndex(int col) const;
    void showImageProperty(const ImageEntry* ie);
    void showItemProperty(const FileItem* item);
    void applySort();

    wxWindow* m_contentParent{nullptr};
    wxSplitterWindow* m_splitter{nullptr};
    wxTreeCtrl* m_tree{nullptr};
    wxGrid* m_grid{nullptr};
    wxPanel* m_right_panel{nullptr};

    std::unique_ptr<LibraryModel> m_model;
    std::vector<std::string> m_current_pkg;
    std::vector<std::string> m_visible_styles;
    std::vector<bool> m_style_visible;
    int m_sort_col{-1};
    bool m_sort_asc{true};
    SortMethod m_sort_method{SORT_NAME};
    std::vector<FileItem> m_sorted_items;
    mutable std::map<std::string, wxBitmap> m_thumb_cache;

    static UIStateValueDescriptor s_sortMethods[3];
};

class TSBFrame : public uiFrame {
  public:
    explicit TSBFrame(const wxString& title);
    ~TSBFrame() override = default;

    void openLibrary(const wxString& path);
    void refreshLibrary();

  private:
    TSBCore m_core;
};

} // namespace tsb

#endif
