/////////////////////////////////////////////////////////////////////////////
// Name:        include/PDFViewDocumentFrame.h
// Purpose:     wxPDFViewDocumentFrame class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_DOCUMENT_FRAME_H
#define PDFVIEW_DOCUMENT_FRAME_H

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/srchctrl.h>
#include <wx/timer.h>
#include <wx/artprov.h>
#include <wx/infobar.h>

#include "PDFView.h"
#include "PDFViewBookmarksCtrl.h"
#include "PDFViewThumbnailListBox.h"

class wxPDFViewActivityPanel;

/**
   PDF reader frame combining all controls to a ready built PDF reader

   This frame combines the other controls into a PDF reader frame. It can
   be a good base for deriving your own frame or directly using it in your
   application. Simply initiate an instance load a file into it and show it.

   Example:
   @code
   wxPDFViewDocumentFrame* docFrame = new wxPDFViewDocumentFrame(this);
   docFrame->LoadFile("C:\\mydocument.pdf");
   docFrame->Show();
   @endcode

   @see wxPDFView, wxPDFViewBookmarksCtrl, wxPDFViewThumbnailListBox
*/
class wxPDFViewDocumentFrame: public wxFrame
{
public:
	static wxString DefaultName;
	
	wxPDFViewDocumentFrame() { }

	wxPDFViewDocumentFrame(wxWindow* parent, 
		wxWindowID id = wxID_ANY,
		const wxString& title = wxString(),
		const wxPoint &pos=wxDefaultPosition, 
		const wxSize &size=wxDefaultSize, 
		long style=wxDEFAULT_FRAME_STYLE, 
		const wxString &name=DefaultName);

	~wxPDFViewDocumentFrame();

	bool Create(wxWindow* parent, 
		wxWindowID id = wxID_ANY,
		const wxString& title = wxString(),
		const wxPoint &pos=wxDefaultPosition, 
		const wxSize &size=wxDefaultSize, 
		long style=wxDEFAULT_FRAME_STYLE, 
		const wxString &name=DefaultName);

	/**
	   Load the specified file into the frame

	   @param fileName Fully qualified local file name
	*/
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
	wxInfoBar* m_infoBar;

	wxSearchCtrl* m_searchCtrl;
	wxTextCtrl* m_pageTxtCtrl;
	wxStaticText* m_pageCountTxtCtrl;
	wxComboBox* m_zoomComboBox;
	wxString m_fileName;

	/**
	   Override this method to customize the printDialogData used when the print button is pressed
	*/
	virtual void PreparePrintDialogData(wxPrintDialogData& printDialogData) { }
	
	/**
	   Returns bitmap for the specified art ID
	*/
	virtual wxBitmap GetToolbarBitmap(wxArtID id);
	
	void StartActivity(const wxString& description);
	
	void UpdateActivity(const wxString& description);
	
	void EndActivity();
	
private:
	wxString m_searchText;
	wxPDFViewActivityPanel* m_activityPanel;
	bool m_activityActive;
	
	void OnFrameSize(wxSizeEvent& event);
	void OnFrameClose(wxCloseEvent& event);

	void OnNavigationClick(wxCommandEvent& event);

	void OnPrintClicked(wxCommandEvent& event);

	void OnPageNextClick( wxCommandEvent& event );
	void OnPagePrevClick( wxCommandEvent& event );
	void OnPageTextEnter( wxCommandEvent& event );

	void OnZoomInClick( wxCommandEvent& event );
	void OnZoomOutClick( wxCommandEvent& event );
	void OnZoomTextEnter( wxCommandEvent& event );

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
	void OnPDFRemoteGoto(wxCommandEvent& event);
	void OnPDFLaunch(wxCommandEvent& event);
	void OnPDFUnsupportedFeature(wxCommandEvent& event);
	void OnPDFActivity(wxCommandEvent& event);

	void Find(const wxString& text, bool forward);
	void UpdateSearchControls();

	void StartPrint();
	
	void SaveZoomConfig();
	
	void ShowNavigationPane(bool show);
};


#endif // PDFVIEW_DOCUMENT_FRAME_H
