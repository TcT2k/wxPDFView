#ifndef PDFVIEW_THUMBNAIL_LISTBOX_IMPL_H
#define PDFVIEW_THUMBNAIL_LISTBOX_IMPL_H

#include "PDFViewThumbnailListBox.h"
#include "PDFViewPages.h"

class wxPDFViewThumbnailListBoxImpl
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
	wxPDFViewPages m_pages;

	void OnPDFDocumentReady(wxCommandEvent& event);

	void OnPDFPageChanged(wxCommandEvent& event);

	void OnSelectionChanged(wxCommandEvent& event);

	void OnPageUpdate(wxThreadEvent& event);

	void UpdateVisiblePages();
};

#endif // PDFVIEW_THUMBNAIL_LISTBOX_IMPL_H
