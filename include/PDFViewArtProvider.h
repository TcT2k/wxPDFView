/////////////////////////////////////////////////////////////////////////////
// Name:        include/PDFViewArtProvider.h
// Purpose:     wxPDFViewArtProvider class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_ART_PROVIDER_H
#define PDFVIEW_ART_PROVIDER_H

#include <wx/artprov.h>

// Art IDs
#define wxART_PDFVIEW_PAGE_FIT		wxART_MAKE_ART_ID(wxART_PDFVIEW_PAGE_FIT)
#define wxART_PDFVIEW_PAGE_WIDTH	wxART_MAKE_ART_ID(wxART_PDFVIEW_PAGE_WIDTH)
#define wxART_PDFVIEW_ZOOM_IN		wxART_MAKE_ART_ID(wxART_PDFVIEW_ZOOM_IN)
#define wxART_PDFVIEW_ZOOM_OUT		wxART_MAKE_ART_ID(wxART_PDFVIEW_ZOOM_OUT)

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
