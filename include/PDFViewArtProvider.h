#ifndef PDFVIEW_ART_PROVIDER_H
#define PDFVIEW_ART_PROVIDER_H

#include <wx/artprov.h>

// Art IDs
#define wxART_PDFVIEW_PAGE_FIT		wxART_MAKE_ART_ID(wxART_PDFVIEW_PAGE_FIT)
#define wxART_PDFVIEW_PAGE_WIDTH	wxART_MAKE_ART_ID(wxART_PDFVIEW_PAGE_WIDTH)

class wxPDFViewArtProvider: public wxArtProvider
{
public:
	wxPDFViewArtProvider();

	static void Initialize();

protected:
	virtual wxBitmap CreateBitmap(const wxArtID& id,
								  const wxArtClient& client,
								  const wxSize& sizeHint);
private:
	static bool m_registered;

};

#endif // PDFVIEW_ART_PROVIDER_H
