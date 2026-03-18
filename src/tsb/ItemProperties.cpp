#include "ItemProperties.hpp"

#include <wx/clipbrd.h>
#include <wx/statline.h>
#include <wx/textctrl.h>

namespace tsb {

ItemPropertiesDialog::ItemPropertiesDialog(wxWindow* parent, const wxString& title,
                                           const wxString& name, const FileItem* item)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(440, 400),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    m_name = name;
    m_item = item;

    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
    wxString nameStr = wxString::FromUTF8(item->name.c_str());
    wxString fmtStr;
    for (size_t i = 0; i < item->formats.size(); ++i) {
        if (i)
            fmtStr += ", ";
        int cnt = 1;
        auto it = item->format_counts.find(item->formats[i]);
        if (it != item->format_counts.end())
            cnt = it->second;
        fmtStr += wxString::Format("%d %s", cnt, wxString::FromUTF8(item->formats[i].c_str()));
    }
    wxString line2 = wxString::Format("%d styles in %zu formats: %s", item->count,
                                      item->formats.size(), fmtStr.c_str());
    wxString line3;
    if (item->total_bytes >= 1024 * 1024)
        line3 = wxString::Format("%.1f M total", item->total_bytes / (1024.0 * 1024.0));
    else if (item->total_bytes >= 1024)
        line3 = wxString::Format("%.1f k total", item->total_bytes / 1024.0);
    else
        line3 = wxString::Format("%lld total", (long long)item->total_bytes);
    top->Add(new wxStaticText(this, wxID_ANY, nameStr), 0, wxALL, 5);
    top->Add(new wxStaticText(this, wxID_ANY, line2), 0, wxALL, 5);
    top->Add(new wxStaticText(this, wxID_ANY, line3), 0, wxALL, 5);
    top->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 5);
    top->Add(new wxStaticText(this, wxID_ANY, "Path Info:"), 0, wxALL, 2);
    wxBoxSizer* pathRow = new wxBoxSizer(wxHORIZONTAL);
    pathRow->Add(20, 0);
    pathRow->Add(new wxStaticText(this, wxID_ANY, nameStr), 1, wxALIGN_CENTER_VERTICAL);
    wxButton* copyPath = new wxButton(this, wxID_ANY, "Copy");
    pathRow->Add(copyPath, 0, wxALL, 2);
    top->Add(pathRow, 0, wxEXPAND | wxALL, 5);
    top->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 5);
    top->Add(new wxStaticText(this, wxID_ANY, "Code Snippets:"), 0, wxALL, 2);
    wxChoice* toolChoice = new wxChoice(this, wxID_ANY);
    toolChoice->Append("bas-ui icon ImageSet");
    toolChoice->SetSelection(0);
    top->Add(toolChoice, 0, wxALL, 5);
    std::string snippet;
    for (const auto& im : item->images) {
        std::string var = im.themestyle_id;
        for (char& c : var)
            if (c == '/')
                c = '_';
        snippet +=
            "ImageSet(\"" + item->name + "\", ThemeStyles." + var + ", \"" + im.rel_path + "\");\n";
    }
    wxTextCtrl* snippetText =
        new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(snippet.c_str()), wxDefaultPosition,
                       wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    top->Add(snippetText, 1, wxEXPAND | wxALL, 5);
    wxButton* copySnippet = new wxButton(this, wxID_ANY, "Copy");
    top->Add(copySnippet, 0, wxALL, 5);
    top->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 5);
    wxBoxSizer* btnRow = new wxBoxSizer(wxHORIZONTAL);
    btnRow->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 5);
    btnRow->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALL, 5);
    top->Add(btnRow, 0, wxALIGN_RIGHT | wxALL, 5);
    this->SetSizerAndFit(top);
    this->SetMinSize(wxSize(380, 300));
    copyPath->Bind(wxEVT_BUTTON, [nameStr](wxCommandEvent&) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(nameStr));
            wxTheClipboard->Close();
        }
    });
    copySnippet->Bind(wxEVT_BUTTON, [snippetText](wxCommandEvent&) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(snippetText->GetValue()));
            wxTheClipboard->Close();
        }
    });
    this->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        if (e.GetId() == wxID_OK || e.GetId() == wxID_CLOSE)
            this->EndModal(e.GetId());
    });
}

} // namespace tsb