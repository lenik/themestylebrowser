#ifndef TSB_ITEM_PROPERTIES_HPP
#define TSB_ITEM_PROPERTIES_HPP

#include "LibraryModel.hpp"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>

namespace tsb {

class ItemPropertiesDialog : public wxDialog {
  public:
    ItemPropertiesDialog(wxWindow* parent, const wxString& title, const wxString& name,
                         const FileItem* item);
    ~ItemPropertiesDialog() override = default;

  private:
    wxString m_name;
    const FileItem* m_item;
};

} // namespace tsb

#endif