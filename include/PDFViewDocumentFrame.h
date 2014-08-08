#ifndef PDFVIEW_DOCUMENT_FRAME_H
#define PDFVIEW_DOCUMENT_FRAME_H

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/srchctrl.h>
#include <wx/timer.h>

#include "PDFView.h"
#include "PDFViewBookmarksCtrl.h"
#include "PDFViewThumbnailListBox.h"

class wxPDFViewDocumentFrame: public wxFrame
{
public:
	wxPDFViewDocumentFrame(wxWindow* parent, 
		const wxPoint &pos=wxDefaultPosition, 
		const wxSize &size=wxDefaultSize, 
		long style=wxDEFAULT_FRAME_STYLE, 
		const wxString &name=wxFrameNameStr);

	~wxPDFViewDocumentFrame();

	bool LoadFile(const wxString& fileName);

	wxPDFView* GetPDFView() { return m_pdfView; };

protected:
	enum
	{
		ID_ZOOM_IN = 1500,
		ID_ZOOM_OUT,
		ID_NAVIGATION,
		ID_ZOOM_PAGE_FIT,
		ID_ZOOM_PAGE_WIDTH,
		ID_FIND_NEXT,
		ID_FIND_PREV
	};

	wxSplitterWindow* m_splitter;
	wxPanel* m_navPanel;
	wxNotebook* m_docNotebook;
	wxPanel* m_bookmarkPanel;
	wxPDFViewBookmarksCtrl* m_pdfViewBookmarksCtrl;
	wxPanel* m_thumbPanel;
	wxPDFViewThumbnailListBox* m_thumbnailListBox;
	wxPanel* m_docPanel;
	wxPDFView* m_pdfView;
	wxToolBar* m_toolBar;

	wxSearchCtrl* m_searchCtrl;
	wxTextCtrl* m_pageTxtCtrl;
	wxStaticText* m_pageCountTxtCtrl;
	wxComboBox* m_zoomComboBox;

private:
	wxString m_searchText;

	void OnNavigationClick(wxCommandEvent& event);

	void OnPrintClicked(wxCommandEvent& event);

	void OnPageNextClick( wxCommandEvent& event );
	void OnPagePrevClick( wxCommandEvent& event );

	void OnZoomInClick( wxCommandEvent& event );
	void OnZoomOutClick( wxCommandEvent& event );

	void OnZoomComboBox( wxCommandEvent& event );

	void OnZoomPageFitClick( wxCommandEvent& event);
	void OnZoomPageWidthClick( wxCommandEvent& event);

	void OnSearchCtrlFind(wxCommandEvent& event);
	void OnSearchCtrlCancel(wxCommandEvent& event);
	void OnSearchCtrlText(wxCommandEvent& event);
	void OnSearchNext(wxCommandEvent& event);
	void OnSearchPrev(wxCommandEvent& event);

	void OnPDFPageChanged(wxCommandEvent& event);
	void OnPDFZoomChanged(wxCommandEvent& event);
	void OnPDFZoomTypeChanged(wxCommandEvent& event);
	void OnPDFDocumentReady(wxCommandEvent& event);
	void OnPDFURLClicked(wxCommandEvent& event);

	void Find(const wxString& text, bool forward);
	void UpdateSearchControls();

	void StartPrint();
};


#endif // PDFVIEW_DOCUMENT_FRAME_H
