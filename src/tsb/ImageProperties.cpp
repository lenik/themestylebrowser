#include "ImageProperties.hpp"

#include <wx/clipbrd.h>
#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/statline.h>

namespace tsb {

ImagePropertiesDialog::ImagePropertiesDialog(wxWindow* parent, const wxString& title,
                                             const wxString& path, const ImageEntry* ie)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(440, 400),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    m_path = path;
    m_styleId = ie->themestyle_id;

    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

    wxString pathRel = wxString::FromUTF8(ie->rel_path.c_str());
    wxString styleId = wxString::FromUTF8(ie->themestyle_id.c_str());

    while (styleId.EndsWith("/"))
        styleId.RemoveLast();
    wxString sizeStr = wxString::Format("%lld bytes", (long long)ie->size);
    if (wxFileName::Exists(wxString::FromUTF8(ie->full_path.c_str()))) {
        wxFileName fn(wxString::FromUTF8(ie->full_path.c_str()));
        wxDateTime modTime = fn.GetModificationTime();
        if (modTime.IsValid())
            sizeStr += "  " + modTime.Format("%Y-%m-%d %H:%M:%S", wxDateTime::Local);
    }
    top->Add(new wxStaticText(this, wxID_ANY, pathRel), 0, wxALL, 5);
    top->Add(new wxStaticText(this, wxID_ANY, sizeStr), 0, wxALL, 5);
    top->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 5);
    top->Add(new wxStaticText(this, wxID_ANY, "Path Info:"), 0, wxALL, 2);
    wxBoxSizer* pathRow = new wxBoxSizer(wxHORIZONTAL);
    pathRow->Add(20, 0);
    pathRow->Add(new wxStaticText(this, wxID_ANY, pathRel), 1, wxALIGN_CENTER_VERTICAL);
    wxButton* copyPath = new wxButton(this, wxID_ANY, "Copy");
    pathRow->Add(copyPath, 0, wxALL, 2);
    top->Add(pathRow, 0, wxEXPAND | wxALL, 5);
    top->Add(new wxStaticText(this, wxID_ANY, "Theme Style:"), 0, wxALL, 2);
    wxBoxSizer* styleRow = new wxBoxSizer(wxHORIZONTAL);
    styleRow->Add(20, 0);
    styleRow->Add(new wxStaticText(this, wxID_ANY, styleId), 1, wxALIGN_CENTER_VERTICAL);
    wxButton* copyStyle = new wxButton(this, wxID_ANY, "Copy");
    styleRow->Add(copyStyle, 0, wxALL, 2);
    top->Add(styleRow, 0, wxEXPAND | wxALL, 5);
    top->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 5);
    top->Add(new wxStaticText(this, wxID_ANY, "Code Snippets:"), 0, wxALL, 2);
    wxChoice* toolChoice = new wxChoice(this, wxID_ANY);
    toolChoice->Append("bas-ui icon ImageSet");
    toolChoice->SetSelection(0);
    top->Add(toolChoice, 0, wxALL, 5);
    std::string rel = ie->rel_path;
    size_t slash = rel.rfind('/');
    std::string fn = (slash != std::string::npos) ? rel.substr(slash + 1) : rel;
    size_t dot = fn.rfind('.');
    std::string base = (dot != std::string::npos) ? fn.substr(0, dot) : fn;
    std::string snippet =
        "ImageSet(\"" + base + "\", \"" + ie->themestyle_id + "\", \"" + fn + "\");";
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
    this->SetMinSize(wxSize(380, 320));
    copyPath->Bind(wxEVT_BUTTON, [pathRel](wxCommandEvent&) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(pathRel));
            wxTheClipboard->Close();
        }
    });
    copyStyle->Bind(wxEVT_BUTTON, [styleId](wxCommandEvent&) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(styleId));
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