/////////////////////////////////////////////////////////////////////////////
// Name:        include/PDFViewThumbnailListBox.h
// Purpose:     wxPDFViewThumbnailListBox class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_THUMBNAIL_LISTBOX_H
#define PDFVIEW_THUMBNAIL_LISTBOX_H

#include <wx/wx.h>
#include <wx/vlbox.h>

#include "PDFView.h"

class wxPDFViewThumbnailListBoxImpl;

/**
   PDF document thumbnails list box

   Has to be connected with a wxPDFView with a call to SetPDFView().
   It will automatically update from navigation in the wxPDFView.

   @see wxPDFView
*/
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

	~wxPDFViewThumbnailListBox();

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
