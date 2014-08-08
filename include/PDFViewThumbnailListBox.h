#ifndef PDFVIEW_THUMBNAIL_LISTBOX_H
#define PDFVIEW_THUMBNAIL_LISTBOX_H

#include <wx/wx.h>
#include <wx/vlbox.h>

#include "PDFView.h"

class wxPDFViewThumbnailListBoxImpl;

class wxPDFViewThumbnailListBox: public wxVListBox
{
public:
	wxPDFViewThumbnailListBox() { }

	wxPDFViewThumbnailListBox(wxWindow *parent,
			   wxWindowID id = wxID_ANY,
			   const wxPoint& pos = wxDefaultPosition,
			   const wxSize& size = wxDefaultSize,
			   long style = 0,
			   const wxString& name = wxVListBoxNameStr)
	{
		Create(parent, id, pos, size, style, name);
	}

	bool Create(wxWindow *parent,
			   wxWindowID id = wxID_ANY,
			   const wxPoint& pos = wxDefaultPosition,
			   const wxSize& size = wxDefaultSize,
			   long style = 0,
			   const wxString& name = wxVListBoxNameStr);

	void SetPDFView(wxPDFView* pdfView);

	wxPDFView* GetPDFView() const;

	virtual void ScrollWindow(int dx, int dy, const wxRect *rect = NULL);

protected:
	virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;

	virtual wxCoord OnMeasureItem(size_t n) const;

private:
	wxPDFViewThumbnailListBoxImpl* m_impl;

	friend class wxPDFViewThumbnailListBoxImpl;
};

#endif // PDFVIEW_THUMBNAIL_LISTBOX_H
