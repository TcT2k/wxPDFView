/////////////////////////////////////////////////////////////////////////////
// Name:        include/private/PDFViewImpl.h
// Purpose:     wxPDFViewImpl class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_IMPL_H
#define PDFVIEW_IMPL_H

#include <wx/sharedptr.h>
#include <wx/graphics.h>
#include <wx/atomic.h>
#include <wx/vector.h>
#include <wx/event.h>
#include <istream>

#include "fpdf_dataavail.h"
#include "fpdfview.h"
#include "fpdf_doc.h"
#include "fpdf_formfill.h"

#include "PDFView.h"
#include "PDFViewPages.h"
#include "PDFViewBookmarks.h"
#include "PDFViewTextRange.h"

class wxPDFViewImpl: public wxPDFViewPagesClient, public wxEvtHandler
{
public:
	wxPDFViewImpl(wxPDFView* ctrl);

	~wxPDFViewImpl();

	void NavigateToPage(wxPDFViewPageNavigation pageNavigation);

	int GetPageCount() const { return (int) m_pages.size(); };

	const wxPDFViewBookmark* GetRootBookmark() const { return (m_bookmarks) ? m_bookmarks->GetRoot() : NULL; };

	void GoToPage(int pageIndex, const wxRect* centerRect);
	
	void GoToPage(int pageIndex);

	void GoToRemote(const wxString& path);

	int GetMostVisiblePage() const { return m_mostVisiblePage; };

	void SetZoom(double zoom);

	double GetZoom() const { return m_zoom; };

	int GetMaxZoom() const { return m_maxZoom; };

	int GetMinZoom() const { return m_minZoom; };

	void SetZoomType(wxPDFViewZoomType zoomType);

	wxPDFViewZoomType GetZoomType() const { return m_zoomType; };

	void SetDisplayFlags(int flags);

	int GetDisplayFlags() const;

	void SetOrientation(wxPDFViewPageOrientation orientation);

	wxPDFViewPageOrientation GetOrientation() const { return m_orientation; };
	
	long Find(const wxString& text, int flags);

	bool IsPrintAllowed() const;

	wxPrintout* CreatePrintout() const;

	wxPrintDialogData GetPrintDialogData() const;

	void Print();

	void StopFind();

	const wxString& GetDocumentTitle() const;

	bool LoadStream(wxSharedPtr<std::istream> pStream, const wxString& password);

	void CloseDocument();

	void HandleScrollWindow(int dx, int dy);

	std::istream* GetStream() const { return m_pDataStream.get(); };

	FPDF_DOCUMENT GetDocument() const { return m_pdfDoc; };

	wxPDFViewPages* GetPages() { return &m_pages; };

	wxSize GetPageSize(int pageIndex) const;

	wxPDFViewPagePosition GetPagePosition(int pageIndex) const;

	void HandleUnsupportedFeature(int type);
	
	void SendActivity(const wxString& description);
	
	void SetPrintValidator(wxPDFViewPrintValidator* validator)
	{
		m_printValidator = validator;
	}
	
	wxPDFViewPrintValidator* GetPrintValidator() const
	{
		return m_printValidator;
	}
	
	void DoGoToAction(int pageIndex);

	void InvalidatePage(int pageIndex, const wxRect* rect = NULL);

	void SetDefaultCursor(wxStockCursor cursor);

	wxPDFView* GetCtrl() const { return m_ctrl; }

	void ExecuteNamedAction(const wxString& action);

private:
	wxPDFView* m_ctrl;
	wxPDFViewBookmarks* m_bookmarks;
	wxPDFViewPrintValidator* m_printValidator;

	// Data source
	wxSharedPtr<std::istream> m_pDataStream;
	FPDF_DOCUMENT m_pdfDoc;
	FPDF_FORMHANDLE m_pdfForm;
	FPDF_AVAIL m_pdfAvail;
	IPDF_JSPLATFORM m_platformCallbacks;
	FPDF_FORMFILLINFO m_formCallbacks;

	// PDF SDK Structures
	static wxAtomicInt ms_pdfSDKRefCount;
	static bool ms_v8initialized;
	FPDF_FILEACCESS m_pdfFileAccess;
	FX_FILEAVAIL m_pdfFileAvail;
	FX_DOWNLOADHINTS m_hints;

	// Document information
	wxVector<wxRect> m_pageRects;
	wxPDFViewPages m_pages;
	wxSize m_docSize;
	int m_maxPageHeight;
	unsigned long m_docPermissions;
	wxString m_documentTitle;
	bool m_linearized;
	int m_backPage;
	wxString m_prevLoadPassword;

	// Display settings
	wxPDFViewZoomType m_zoomType;
	int m_displayFlags;
	int m_pagePadding;
	int m_scrollStepX;
	int m_scrollStepY;
	double m_zoom;
	double m_maxZoom;
	double m_minZoom;
	int m_mostVisiblePage;
	wxCursor m_handCursor;
	wxStockCursor m_defaultCursor;
	wxVector<wxPDFViewTextRange> m_selection;
	wxPDFViewPageOrientation m_orientation;

	// Find information
	wxString m_findText;
	wxVector<wxPDFViewTextRange> m_findResults;
	int m_nextPageToSearch;
	int m_lastPageToSearch;
	int m_lastCharacterIndexToSearch;
	int m_currentFindIndex;

	void UpdateDocumentInfo();
	
	void RecalculatePageRects();

	void AlignPageRects();

	void OnPaint(wxPaintEvent& event);

	void OnSize(wxSizeEvent& event);

	void OnMouseWheel(wxMouseEvent& event);

	void OnMouseMotion(wxMouseEvent& event);

	void OnMouseLeftDown(wxMouseEvent& event);

	void OnMouseLeftUp(wxMouseEvent& event);

	void OnKeyUp(wxKeyEvent& event);

	void OnKeyDown(wxKeyEvent& event);

	void OnKeyChar(wxKeyEvent& event);

	void UpdateVirtualSize();

	wxRect UnscaledToScaled(const wxRect& rect) const;

	wxRect ScaledToUnscaled(const wxRect& rect) const;

	int ClientToPage(const wxPoint& clientPos, wxPoint& pagePos);

	// Returns true if a link is found at the specified position. 
	// If performAction is true the link action will be performed
	bool EvaluateLinkTargetPageAtClientPos(const wxPoint& clientPos, int evtType);

	void CalcVisiblePages();

	void CalcZoomLevel();

	void AddFindResult(const wxPDFViewTextRange& result);

	int FindOnPage(int pageIndex, bool caseSensitive, bool firstSearch, int characterToStartSearchingFrom);

	// Request redraw for specified page
	void RefreshPage(int pageIndex);
	
	static bool AcquireSDK();

	static void ReleaseSDK();

	friend class wxPDFViewPrintout;
};

#endif // PDFVIEW_IMPL_H