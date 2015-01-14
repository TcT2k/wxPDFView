/////////////////////////////////////////////////////////////////////////////
// Name:        include/PDFViewBookmarksCtrl.h
// Purpose:     wxPDFViewBookmarksCtrl class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_BOOKMARKS_CTRL_H
#define PDFVIEW_BOOKMARKS_CTRL_H

#include <wx/dataview.h>

#include "PDFView.h"

/**
   Tree control that display the bookmarks defined in a PDF Document.

   The control has the be linked with a wxPDFView by calling SetPDFView().

   This control will automatically navigate to bookmarks if selected and will update 
   the bookmark list when documents are loaded into the wxPDFView.

   @see wxPDFView
*/
class wxPDFViewBookmarksCtrl: public wxDataViewCtrl
{
public:
	wxPDFViewBookmarksCtrl() { }

	wxPDFViewBookmarksCtrl(wxWindow *parent,
							wxWindowID id,
							const wxPoint& pos = wxDefaultPosition,
							const wxSize& size = wxDefaultSize,
							long style = wxDV_ROW_LINES,
							const wxValidator& validator = wxDefaultValidator)
	{
		Create(parent, id, pos, size, style, validator);
	};

	bool Create(wxWindow *parent,
			wxWindowID id,
			const wxPoint& pos = wxDefaultPosition,
			const wxSize& size = wxDefaultSize,
			long style = wxDV_ROW_LINES,
			const wxValidator& validator = wxDefaultValidator);

	/**
	   Connect this control to a wxPDFView

	   @see wxPDFView
	*/
	void SetPDFView(wxPDFView* pdfView);

	/**
	   @return wxPDFView previously connected with SetPDFView()
	*/
	wxPDFView* GetPDFView() const { return m_pdfView; };

protected:

private:
	wxPDFView* m_pdfView;

	void OnPDFDocumentReady(wxCommandEvent& event);

	void OnPDFDocumentClosed(wxCommandEvent& event);

	void OnSelectionChanged(wxDataViewEvent& event);

	void OnSize(wxSizeEvent& event);
};

#endif // PDFVIEW_BOOKMARKS_CTRL_H
