#ifndef TSB_IMAGE_PROPERTIES_HPP
#define TSB_IMAGE_PROPERTIES_HPP

#include "LibraryModel.hpp"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>

namespace tsb {

class ImagePropertiesDialog : public wxDialog {
  public:
    ImagePropertiesDialog(wxWindow* parent, const wxString& title, const wxString& path,
                          const ImageEntry* ie);
    ~ImagePropertiesDialog() override = default;

  private:
    wxString m_path;
    wxString m_styleId;
};

} // namespace tsb

#endif