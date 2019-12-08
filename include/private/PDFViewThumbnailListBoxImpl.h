/////////////////////////////////////////////////////////////////////////////
// Name:        include/private/PDFViewThumbnailListBoxImpl.h
// Purpose:     wxPDFViewThumbnailListBoxImpl class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_THUMBNAIL_LISTBOX_IMPL_H
#define PDFVIEW_THUMBNAIL_LISTBOX_IMPL_H

#include "PDFViewThumbnailListBox.h"
#include "PDFViewPages.h"

class wxPDFViewThumbnailListBoxImpl: public wxPDFViewPagesClient
{
public:
	wxPDFViewThumbnailListBoxImpl(wxPDFViewThumbnailListBox* ctrl);

	void SetPDFView(wxPDFView* pdfView);

	wxPDFView* GetPDFView() const { return m_pdfView; };

	void DrawPage(wxDC& dc, const wxRect& rect, int pageIndex);

	void HandleScrollWindow(int dx, int dy);

private:
	wxPDFViewThumbnailListBox* m_ctrl;
	wxPDFView* m_pdfView;

	void OnPDFDocumentReady(wxCommandEvent& event);

	void OnPDFDocumentClosed(wxCommandEvent& event);

	void OnPDFPageChanged(wxCommandEvent& event);
	
	void OnPDFOrientationChanged(wxCommandEvent& event);

	void OnSelectionChanged(wxCommandEvent& event);

	void UpdateVisiblePages();
};

#endif // PDFVIEW_THUMBNAIL_LISTBOX_IMPL_H
