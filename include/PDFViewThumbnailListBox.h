#ifndef PDFVIEW_THUMBNAIL_LISTBOX_H
#define PDFVIEW_THUMBNAIL_LISTBOX_H

#include <wx/wx.h>
#include <wx/vlbox.h>

#include "PDFView.h"

class wxPDFViewThumbnailListBox: public wxVListBox
{
public:
	wxPDFViewThumbnailListBox(wxWindow *parent,
			   wxWindowID id = wxID_ANY,
			   const wxPoint& pos = wxDefaultPosition,
			   const wxSize& size = wxDefaultSize,
			   long style = 0,
			   const wxString& name = wxVListBoxNameStr);

	void SetPDFView(wxPDFView* pdfView);

	wxPDFView* GetPDFView() const { return m_pdfView; };

protected:
	virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;

	virtual wxCoord OnMeasureItem(size_t n) const;

private:
	wxPDFView* m_pdfView;

	void Init();

	void OnPDFDocumentReady(wxCommandEvent& event);

	void OnPDFPageChanged(wxCommandEvent& event);

	void OnSelectionChanged(wxCommandEvent& event);

};

#endif // PDFVIEW_THUMBNAIL_LISTBOX_H
