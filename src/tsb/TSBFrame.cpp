#include "tsb/TSBFrame.hpp"

#include "ImageProperties.hpp"
#include "ItemProperties.hpp"

#include <bas/ui/arch/ImageSet.hpp>
#include <bas/ui/arch/UIState.hpp>
#include <bas/wx/images.hpp>

#include <algorithm>
#include <functional>
#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/datetime.h>
#include <wx/dc.h>
#include <wx/defs.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/generic/grid.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>

namespace tsb {

struct TreeItemData : wxTreeItemData {
    explicit TreeItemData(std::string seg) : segment(std::move(seg)) {}
    std::string segment;
};

class ThumbnailCellRenderer : public wxGridCellRenderer {
  public:
    explicit ThumbnailCellRenderer(TSBCore* core) : m_core(core) {}
    void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col,
              bool isSelected) override {
        wxGridCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
        wxString val = grid.GetCellValue(row, col);
        if (val == "-") {
            dc.SetFont(attr.GetFont());
            dc.SetTextForeground(attr.GetTextColour());
            dc.DrawText("-", rect.x + 2, rect.y + (rect.height - dc.GetCharHeight()) / 2);
            return;
        }
        std::string path(val.ToUTF8().data());
        wxBitmap bmp = m_core->loadThumbnail(path, 32);
        if (bmp.IsOk()) {
            int x = rect.x + (rect.width - bmp.GetWidth()) / 2;
            int y = rect.y + (rect.height - bmp.GetHeight()) / 2;
            dc.DrawBitmap(bmp, x, y, true);
        } else {
            dc.SetFont(attr.GetFont());
            dc.SetTextForeground(attr.GetTextColour());
            dc.DrawText(val, rect.x + 2, rect.y + (rect.height - dc.GetCharHeight()) / 2);
        }
    }
    wxGridCellRenderer* Clone() const override { return new ThumbnailCellRenderer(m_core); }
    wxSize GetBestSize(wxGrid&, wxGridCellAttr&, wxDC&, int, int) override {
        return wxSize(40, 40);
    }

  private:
    TSBCore* m_core;
};

enum {
    ID_OPEN_LIBRARY = uiFrame::ID_APP_HIGHEST + 1,
    ID_TREE,
    ID_GRID,
    ID_COL_VIS_BASE = 30000,
    ID_DELETE_IMAGE,
    ID_DELETE_ITEM,
    ID_RENAME_ITEM,
    ID_SORT_METHOD,
};

TSBFrame::TSBFrame(const wxString& title) : uiFrame(title) {
    addFragment(&m_core);
    createView();
}

void TSBFrame::openLibrary(const wxString& path) { m_core.openLibrary(path); }
void TSBFrame::refreshLibrary() { m_core.refreshLibrary(); }

namespace {
constexpr const char* k_stream_if = "streamline-vectors/core/pop/interface-essential";
constexpr const char* k_stream_dev = "streamline-vectors/core/pop/computer-devices";
} // namespace

UIStateValueDescriptor TSBCore::s_sortMethods[3] = {
    {.label = "Sort by &Name",
     .description = "Tree and file list by name ascending (default)",
     .icon = ImageSet(wxART_GO_UP, k_stream_if, "ascending-number-order.svg")},
    {.label = "Sort by &Count",
     .description = "Tree by item/file count descending, list by file count descending",
     .icon = ImageSet(wxART_GO_DOWN, k_stream_if, "descending-number-order.svg")},
    {.label = "Sort by &Size",
     .description = "Tree and file list by size descending",
     .icon = ImageSet(wxART_CDROM, k_stream_dev, "hard-disk.svg")},
};

TSBCore::TSBCore() {
    std::string stream_if = "streamline-vectors/core/pop/interface-essential";
    std::string stream_travel = "streamline-vectors/core/pop/map-travel";

    int seq = 0;
    action(ID_OPEN_LIBRARY, "file", "open_library", seq++, "&Open library...\tCtrl+O",
           "Open theme style library root")
        .icon(wxART_FOLDER_OPEN, stream_if, "open-book.svg")
        .performFn([this](PerformContext*) { onOpenLibrary(nullptr); })
        .install();
    action(wxID_EXIT, "file", "exit", seq++, "E&xit\tAlt+F4", "Quit")
        .icon(wxART_CROSS_MARK, stream_travel, "emergency-exit.svg")
        .performFn([this](PerformContext*) { onExit(nullptr); })
        .install();

    seq = 0;
    state(ID_SORT_METHOD, "view", "sort_method", seq++, "&Sort",
          "Sort order for tree and file list")
        .icon(wxART_GO_DOWN, stream_if, "sort-descending.svg")
        .stateType(UIStateType::ENUM)
        .enumValues({SORT_NAME, SORT_COUNT, SORT_SIZE})
        .valueDescriptorFn([this](int value) { return s_sortMethods[value]; })
        .initValue(SORT_NAME)
        .connect([this](UIStateVariant const value, UIStateVariant const) {
            SortMethod method = static_cast<SortMethod>(std::get<int>(value));
            m_sort_method = method;
            m_sort_col = -1;
            // sort order is ascending for name, descending for count and size
            m_sort_asc = (method == SORT_NAME) ? true : false;
            buildPackageTree();
            refreshFileList();
        })
        .install();
}

void TSBCore::createFragmentView(CreateViewContext* ctx) {
    m_contentParent = ctx->getParent();
    const wxPoint& pos = ctx->getPos();
    const wxSize& size = ctx->getSize();

    m_splitter = new wxSplitterWindow(m_contentParent, wxID_ANY, pos, size, wxSP_3D);
    m_tree = new wxTreeCtrl(m_splitter, ID_TREE, wxDefaultPosition, wxDefaultSize,
                            wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_SINGLE);
    m_tree->AddRoot("Library");
    wxImageList* treeIcons = new wxImageList(16, 16);
    treeIcons->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
    treeIcons->Add(wxArtProvider::GetBitmap(wxART_FOLDER_OPEN, wxART_OTHER, wxSize(16, 16)));
    m_tree->AssignImageList(treeIcons);

    m_right_panel = new wxPanel(m_splitter, wxID_ANY);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    m_grid = new wxGrid(m_right_panel, ID_GRID, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
    m_grid->CreateGrid(1, 1);
    m_grid->EnableGridLines(true);
    m_grid->SetColLabelSize(44);
    m_grid->SetRowLabelSize(40);
    m_grid->SetScrollbars(1, 1, 1, 1);
    m_grid->Bind(wxEVT_GRID_CELL_LEFT_DCLICK, &TSBCore::onGridCellLeftDClick, this);
    m_grid->Bind(wxEVT_GRID_CELL_RIGHT_CLICK, &TSBCore::onGridCellRightClick, this);
    m_grid->Bind(wxEVT_GRID_LABEL_RIGHT_CLICK, &TSBCore::onGridLabelRightClick, this);
    m_grid->Bind(wxEVT_GRID_LABEL_LEFT_CLICK, &TSBCore::onGridLabelLeftClick, this);
    m_grid->Bind(wxEVT_MENU, [this](wxCommandEvent& e) { onDeleteImage(e); }, ID_DELETE_IMAGE);
    m_grid->Bind(wxEVT_MENU, [this](wxCommandEvent& e) { onDeleteItem(e); }, ID_DELETE_ITEM);
    m_grid->Bind(wxEVT_MENU, [this](wxCommandEvent& e) { onRenameItem(e); }, ID_RENAME_ITEM);
    m_grid->Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
        if (e.GetId() >= ID_COL_VIS_BASE)
            onColVisibilityMenu(e);
    });
    rightSizer->Add(m_grid, 1, wxEXPAND);
    m_right_panel->SetSizer(rightSizer);

    m_splitter->SplitVertically(m_tree, m_right_panel, 220);
    m_splitter->SetMinimumPaneSize(100);

    m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &TSBCore::onTreeSelChanged, this);
    m_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &TSBCore::onTreeItemActivated, this);

    m_model = std::make_unique<LibraryModel>();
}

wxEvtHandler* TSBCore::getEventHandler() {
    if (m_contentParent == nullptr) {
        logerror("TSBCore::getEventHandler: m_contentParent is nullptr");
        return nullptr;
    }
    return m_contentParent->GetEventHandler();
}

void TSBCore::openLibrary(const wxString& path) {
    std::string root(path.ToUTF8().data());
    if (!m_model->load(root)) {
        wxMessageBox("Failed to load library. Ensure the directory contains .themestyles and "
                     "theme/style folders.",
                     "Error", wxOK | wxICON_ERROR, m_contentParent);
        return;
    }
    wxWindow* w = m_contentParent;
    while (w && !dynamic_cast<wxFrame*>(w))
        w = w->GetParent();
    if (wxFrame* frame = dynamic_cast<wxFrame*>(w))
        frame->SetTitle(
            wxString::FromUTF8(("Theme Style Browser - " + m_model->getLibraryRoot()).c_str()));
    buildPackageTree();
    m_style_visible.clear();
    m_style_visible.resize(m_model->getThemeStyles().size(), true);
    m_current_pkg.clear();
    m_sort_col = -1;
    refreshThemeStyleColumns();
    refreshFileList();
    // Select first package node if any
    wxTreeItemId rootId = m_tree->GetRootItem();
    wxTreeItemIdValue cookie;
    wxTreeItemId first = m_tree->GetFirstChild(rootId, cookie);
    if (first.IsOk())
        m_tree->SelectItem(first);
}

void TSBCore::refreshLibrary() {
    if (m_model->getLibraryRoot().empty())
        return;
    if (!m_model->load(m_model->getLibraryRoot()))
        return;
    buildPackageTree();
    refreshThemeStyleColumns();
    refreshFileList();
}

std::vector<PackagePath> TSBCore::getSortedDirectChildren(const PackagePath& parent_prefix) const {
    std::vector<PackagePath> out;
    for (const PackagePath& p : m_model->getPackagePaths()) {
        if (p.size() != parent_prefix.size() + 1)
            continue;
        bool match = true;
        for (size_t i = 0; i < parent_prefix.size(); ++i) {
            if (p[i] != parent_prefix[i]) {
                match = false;
                break;
            }
        }
        if (match)
            out.push_back(p);
    }
    SortMethod method = m_sort_method;
    auto* model = m_model.get();
    std::sort(out.begin(), out.end(), [model, method](const PackagePath& a, const PackagePath& b) {
        if (method == SORT_NAME) {
            return a.back() < b.back();
        }
        if (method == SORT_COUNT) {
            int ia = model->getPackageItemCount(a), ib = model->getPackageItemCount(b);
            if (ia != ib)
                return ia > ib;
            int fa = model->getPackageFileCount(a), fb = model->getPackageFileCount(b);
            if (fa != fb)
                return fa > fb;
            return a.back() < b.back();
        }
        // method == SORT_SIZE: size desc
        int64_t sa = model->getPackageTotalSize(a), sb = model->getPackageTotalSize(b);
        if (sa != sb)
            return sa > sb;
        return a.back() < b.back();
    });
    return out;
}

void TSBCore::buildPackageTree() {
    m_tree->DeleteAllItems();
    wxTreeItemId rootId = m_tree->AddRoot("Library");
    std::map<PackagePath, wxTreeItemId> nodeMap;
    nodeMap[{}] = rootId;

    std::function<void(const PackagePath&)> addChildren;
    addChildren = [this, &nodeMap, &addChildren](const PackagePath& parent_prefix) {
        std::vector<PackagePath> children = getSortedDirectChildren(parent_prefix);
        for (const PackagePath& pkg : children) {
            int fc = m_model->getPackageFileCount(pkg);
            int ic = m_model->getPackageItemCount(pkg);
            wxString label =
                wxString::FromUTF8(pkg.back().c_str()) + wxString::Format(" (%d/%d)", fc, ic);
            wxTreeItemId parentId = nodeMap[parent_prefix];
            wxTreeItemId id = m_tree->AppendItem(parentId, label, 0, 1);
            m_tree->SetItemImage(id, 0, wxTreeItemIcon_Normal);
            m_tree->SetItemImage(id, 1, wxTreeItemIcon_Expanded);
            m_tree->SetItemData(id, new TreeItemData(pkg.back()));
            nodeMap[pkg] = id;
            addChildren(pkg);
        }
    };
    addChildren({});
    m_tree->ExpandAll();
}

void TSBCore::onTreeSelChanged(wxTreeEvent& e) {
    wxTreeItemId id = e.GetItem();
    if (!id.IsOk())
        return;
    std::vector<std::string> segments;
    wxTreeItemId cur = id;
    wxTreeItemId root = m_tree->GetRootItem();
    while (cur.IsOk() && cur != root) {
        auto* data = static_cast<TreeItemData*>(m_tree->GetItemData(cur));
        if (data)
            segments.push_back(data->segment);
        else
            break;
        cur = m_tree->GetItemParent(cur);
    }
    std::reverse(segments.begin(), segments.end());
    m_current_pkg = segments;
    refreshThemeStyleColumns();
    refreshFileList();
}

void TSBCore::onTreeItemActivated(wxTreeEvent& e) { onTreeSelChanged(e); }

void TSBCore::selectPackagePath(const std::vector<std::string>& pkg) {
    m_current_pkg = pkg;
    refreshThemeStyleColumns();
    refreshFileList();
}

void TSBCore::refreshThemeStyleColumns() {
    m_visible_styles = m_model->getThemeStylesForPackage(m_current_pkg);
    for (size_t i = 0; i < m_visible_styles.size();) {
        size_t idx_in_all = 0;
        for (const auto& ts : m_model->getThemeStyles()) {
            if (ts.id == m_visible_styles[i])
                break;
            idx_in_all++;
        }
        if (idx_in_all < m_style_visible.size() && !m_style_visible[idx_in_all]) {
            m_visible_styles.erase(m_visible_styles.begin() + i);
        } else {
            i++;
        }
    }
}

void TSBCore::refreshFileList() {
    if (!m_model || m_model->getLibraryRoot().empty())
        return;

    m_sorted_items = m_model->getFileItems(m_current_pkg);
    int sortBy = (m_sort_col >= 0)
                     ? m_sort_col
                     : (m_sort_method == SORT_NAME ? 0 : (m_sort_method == SORT_COUNT ? 1 : 2));
    bool asc = (m_sort_method == SORT_NAME && m_sort_col < 0) || (m_sort_col == 0 && m_sort_asc);
    if (m_sort_method == SORT_COUNT && m_sort_col < 0)
        asc = false;
    if (m_sort_method == SORT_SIZE && m_sort_col < 0)
        asc = false;
    if (sortBy == 0) {
        std::sort(m_sorted_items.begin(), m_sorted_items.end(),
                  [asc](const FileItem& a, const FileItem& b) {
                      int c = a.name.compare(b.name);
                      return asc ? (c < 0) : (c > 0);
                  });
    } else if (sortBy == 1) {
        std::sort(m_sorted_items.begin(), m_sorted_items.end(),
                  [asc](const FileItem& a, const FileItem& b) {
                      return asc ? (a.count < b.count) : (a.count > b.count);
                  });
    } else {
        std::sort(m_sorted_items.begin(), m_sorted_items.end(),
                  [asc](const FileItem& a, const FileItem& b) {
                      return asc ? (a.total_bytes < b.total_bytes)
                                 : (a.total_bytes > b.total_bytes);
                  });
    }

    const std::vector<FileItem>& items = m_sorted_items;
    int cols = 3 + static_cast<int>(m_visible_styles.size()); // Name, Count, Size, style...
    m_grid->BeginBatch();
    if (m_grid->GetNumberCols() != cols) {
        if (m_grid->GetNumberCols() > 0)
            m_grid->DeleteCols(0, m_grid->GetNumberCols());
        m_grid->AppendCols(cols);
        m_grid->SetColSize(COL_NAME, 180);
        m_grid->SetColSize(COL_COUNT, 50);
        m_grid->SetColSize(COL_SIZE, 70);
        for (int c = 3; c < cols; ++c)
            m_grid->SetColSize(c, 44);
    }
    m_grid->SetColLabelValue(COL_NAME, "Name");
    m_grid->SetColLabelValue(COL_COUNT, "Count");
    m_grid->SetColLabelValue(COL_SIZE, "Size");
    for (size_t i = 0; i < m_visible_styles.size(); ++i)
        m_grid->SetColLabelValue(COL_STYLE_BASE + static_cast<int>(i),
                                 wxString::FromUTF8(m_visible_styles[i].c_str()));
    m_grid->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
    m_grid->EnableCellEditControl(false);
    for (int c = COL_STYLE_BASE; c < cols; ++c) {
        wxGridCellAttr* attr = new wxGridCellAttr();
        attr->SetRenderer(new ThumbnailCellRenderer(this));
        m_grid->SetColAttr(c, attr);
    }

    int rows = static_cast<int>(items.size());
    if (m_grid->GetNumberRows() != rows) {
        if (m_grid->GetNumberRows() > rows)
            m_grid->DeleteRows(rows, m_grid->GetNumberRows() - rows);
        else if (m_grid->GetNumberRows() < rows)
            m_grid->AppendRows(rows - m_grid->GetNumberRows());
    }

    for (int r = 0; r < rows; ++r) {
        const FileItem& it = items[r];
        m_grid->SetCellValue(r, COL_NAME, wxString::FromUTF8(it.name.c_str()));
        m_grid->SetCellValue(r, COL_COUNT, wxString::Format("%d", it.count));
        m_grid->SetCellValue(r, COL_SIZE, formatSize(it.total_bytes));
        for (size_t c = 0; c < m_visible_styles.size(); ++c) {
            const std::string& styleId = m_visible_styles[c];
            std::string path;
            for (const auto& im : it.images) {
                if (im.themestyle_id == styleId) {
                    path = im.full_path;
                    break;
                }
            }
            if (path.empty())
                m_grid->SetCellValue(r, COL_STYLE_BASE + static_cast<int>(c), "-");
            else
                m_grid->SetCellValue(r, COL_STYLE_BASE + static_cast<int>(c),
                                     wxString::FromUTF8(path.c_str()));
        }
    }
    m_grid->EndBatch();
}

void TSBCore::applySort() { refreshFileList(); }

wxBitmap TSBCore::loadThumbnail(const std::string& path, int size) {
    std::string key = path + "@" + std::to_string(size);
    auto it = m_thumb_cache.find(key);
    if (it != m_thumb_cache.end())
        return it->second;
    std::optional<wxBitmap> bmp = imageLoadFile(wxString::FromUTF8(path.c_str()), size, size);
    if (!bmp)
        return wxNullBitmap;
    m_thumb_cache[key] = *bmp;
    return *bmp;
}

wxString TSBCore::formatSize(int64_t bytes) {
    if (bytes >= 1024 * 1024)
        return wxString::Format("%.1f M", bytes / (1024.0 * 1024.0));
    if (bytes >= 1024)
        return wxString::Format("%.1f k", bytes / 1024.0);
    return wxString::Format("%lld", (long long)bytes);
}

int TSBCore::getStyleColumnIndex(int col) const {
    if (col < 3)
        return -1;
    int idx = col - COL_STYLE_BASE;
    return (idx >= 0 && idx < static_cast<int>(m_visible_styles.size())) ? idx : -1;
}

void TSBCore::onGridCellLeftDClick(wxGridEvent& e) {
    int row = e.GetRow();
    int col = e.GetCol();
    if (row < 0 || row >= static_cast<int>(m_sorted_items.size()))
        return;
    const FileItem& item = m_sorted_items[row];

    if (col == COL_NAME) {
        showItemProperty(&item);
        return;
    }
    if (col == COL_COUNT || col == COL_SIZE)
        return;

    int styleIdx = getStyleColumnIndex(col);
    if (styleIdx >= 0) {
        wxString val = m_grid->GetCellValue(row, col);
        if (val == "-")
            return;
        std::string pathVal(val.ToUTF8().data());
        for (const auto& im : item.images) {
            if (im.full_path == pathVal) {
                showImageProperty(&im);
                return;
            }
        }
    }
}

void TSBCore::onGridCellRightClick(wxGridEvent& e) {
    int row = e.GetRow();
    int col = e.GetCol();
    wxMenu menu;
    if (col == COL_NAME) {
        menu.Append(ID_DELETE_ITEM, "Delete item group");
        menu.Append(ID_RENAME_ITEM, "Rename item (F2)");
    } else {
        int styleIdx = getStyleColumnIndex(col);
        if (styleIdx >= 0) {
            wxString val = m_grid->GetCellValue(row, col);
            if (val != "-")
                menu.Append(ID_DELETE_IMAGE, "Delete image");
        }
    }
    if (menu.GetMenuItemCount() > 0)
        m_grid->PopupMenu(&menu, e.GetPosition());
}

void TSBCore::onGridLabelRightClick(wxGridEvent& e) {
    if (e.GetRow() >= 0)
        return;
    int col = e.GetCol();
    if (col == COL_NAME) {
        wxMenu menu;
        for (size_t i = 0; i < m_model->getThemeStyles().size(); ++i) {
            const auto& ts = m_model->getThemeStyles()[i];
            int id = ID_COL_VIS_BASE + static_cast<int>(i);
            menu.AppendCheckItem(id, wxString::FromUTF8(ts.id.c_str()));
            menu.Check(id, m_style_visible[i]);
        }
        menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) { onColVisibilityMenu(e); });
        m_grid->PopupMenu(&menu, e.GetPosition());
    } else {
        int styleIdx = getStyleColumnIndex(col);
        if (styleIdx >= 0 && styleIdx < static_cast<int>(m_visible_styles.size())) {
            wxMenu menu;
            const std::string& styleId = m_visible_styles[styleIdx];
            size_t idx_in_all = 0;
            for (const auto& ts : m_model->getThemeStyles()) {
                if (ts.id == styleId)
                    break;
                idx_in_all++;
            }
            if (idx_in_all < m_style_visible.size()) {
                int id = ID_COL_VIS_BASE + static_cast<int>(idx_in_all);
                menu.Append(id, "Hide column '" + wxString::FromUTF8(styleId.c_str()) + "'");
                menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) { onColVisibilityMenu(e); });
                m_grid->PopupMenu(&menu, e.GetPosition());
            }
        }
    }
}

void TSBCore::onGridLabelLeftClick(wxGridEvent& e) {
    if (e.GetRow() >= 0)
        return;
    int col = e.GetCol();
    if (col == COL_NAME) {
        m_sort_col = 0;
        m_sort_asc = !m_sort_asc;
        refreshFileList();
    } else if (col == COL_COUNT) {
        m_sort_col = 1;
        m_sort_asc = false;
        refreshFileList();
    } else if (col == COL_SIZE) {
        m_sort_col = 2;
        m_sort_asc = false;
        refreshFileList();
    }
}

void TSBCore::onColVisibilityMenu(wxCommandEvent& e) {
    int id = e.GetId();
    if (id >= ID_COL_VIS_BASE &&
        id < ID_COL_VIS_BASE + static_cast<int>(m_model->getThemeStyles().size())) {
        size_t idx = id - ID_COL_VIS_BASE;
        if (idx < m_style_visible.size()) {
            m_style_visible[idx] = !m_style_visible[idx];
            refreshThemeStyleColumns();
            refreshFileList();
        }
    }
}

void TSBCore::onOpenLibrary(PerformContext*) {
    wxDirDialog dlg(m_contentParent, "Select library root (directory containing .themestyles)",
                    wxEmptyString, wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK)
        return;
    openLibrary(dlg.GetPath());
}

void TSBCore::onExit(PerformContext*) {
    wxWindow* w = m_contentParent;
    while (w && !dynamic_cast<wxFrame*>(w))
        w = w->GetParent();
    if (wxFrame* frame = dynamic_cast<wxFrame*>(w))
        frame->Close(true);
}

void TSBCore::onDeleteImage(wxCommandEvent&) {
    wxMessageBox("Delete image: not implemented yet.", "Info", wxOK, m_contentParent);
}

void TSBCore::onDeleteItem(wxCommandEvent&) {
    wxMessageBox("Delete item group: not implemented yet.", "Info", wxOK, m_contentParent);
}

void TSBCore::onRenameItem(wxCommandEvent&) {
    wxMessageBox("Rename item: not implemented yet (F2).", "Info", wxOK, m_contentParent);
}

void TSBCore::showImageProperty(const ImageEntry* ie) {
    if (!ie)
        return;
    ImagePropertiesDialog dlg(m_contentParent, "Image Property",
                              wxString::FromUTF8(ie->rel_path.c_str()), ie);
    dlg.ShowModal();
}

void TSBCore::showItemProperty(const FileItem* item) {
    if (!item)
        return;
    ItemPropertiesDialog dlg(m_contentParent, "Item Property",
                             wxString::FromUTF8(item->name.c_str()), item);
    dlg.ShowModal();
}

} // namespace tsb
