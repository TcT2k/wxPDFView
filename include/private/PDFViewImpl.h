#ifndef PDFVIEW_IMPL_H
#define PDFVIEW_IMPL_H

#include <wx/sharedptr.h>
#include <wx/graphics.h>
#include <set>
#include <vector>
#include <istream>

#include "fpdf_dataavail.h"
#include "fpdfview.h"
#include "fpdfdoc.h"
#include "fpdfformfill.h"

#include "PDFView.h"
#include "PDFViewPages.h"
#include "PDFViewBookmarks.h"
#include "PDFViewTextRange.h"

class wxPDFViewImpl
{
public:
	wxPDFViewImpl(wxPDFView* ctrl);

	~wxPDFViewImpl();

	void NavigateToPage(wxPDFViewPageNavigation pageNavigation);

	int GetPageCount() const { return m_pageCount; };

	const wxPDFViewBookmark* GetRootBookmark() const { return (m_bookmarks) ? m_bookmarks->GetRoot() : NULL; };

	void SetCurrentPage(int pageIndex);

	int GetCurrentPage() const;

	void SetZoom(double zoom);

	double GetZoom() const { return m_zoom; };

	int GetMaxZoom() const { return m_maxZoom; };

	int GetMinZoom() const { return m_minZoom; };

	void SetZoomType(wxPDFViewZoomType zoomType);

	wxPDFViewZoomType GetZoomType() const { return m_zoomType; };

	long Find(const wxString& text, int flags);

	void StopFind();

	bool LoadStream(wxSharedPtr<std::istream> pStream);

	void CloseDocument();

	void HandleScrollWindow(int dx, int dy);

	std::istream* GetStream() const { return m_pDataStream.get(); };

	void* GetDocument() const { return m_pdfDoc; };

	wxSize GetPageSize(int pageIndex) const;

private:
	wxPDFView* m_ctrl;
	wxPDFViewBookmarks* m_bookmarks;

	// Data source
	wxSharedPtr<std::istream> m_pDataStream;
	FPDF_DOCUMENT m_pdfDoc;
	FPDF_FORMHANDLE m_pdfForm;
	FPDF_AVAIL m_pdfAvail;

	// PDF SDK Structures
	FPDF_FILEACCESS m_pdfFileAccess;
	FX_FILEAVAIL m_pdfFileAvail;

	// Document information
	int m_pageCount;
	std::vector<wxRect> m_pageRects;
	wxPDFViewPages m_pages;
	wxSize m_docSize;

	// Display settings
	wxPDFViewZoomType m_zoomType;
	int m_pagePadding;
	int m_scrollStepX;
	int m_scrollStepY;
	double m_zoom;
	double m_maxZoom;
	double m_minZoom;
	int m_currentPage;
	int m_firstVisiblePage;
	int m_lastVisiblePage;
	wxCursor m_handCursor;
	wxVector<wxPDFViewTextRange> m_selection;

	// Find information
	wxString m_findText;
	wxVector<wxPDFViewTextRange> m_findResults;
	int m_nextPageToSearch;
	int m_lastPageToSearch;
	int m_lastCharacterIndexToSearch;
	int m_currentFindIndex;

	void UpdateDocumentInfo();

	void AlignPageRects();

	void OnPageUpdate(wxThreadEvent& event);

	void OnPaint(wxPaintEvent& event);

	void OnSize(wxSizeEvent& event);

	void OnMouseWheel(wxMouseEvent& event);

	void OnMouseMotion(wxMouseEvent& event);

	void OnMouseLeftUp(wxMouseEvent& event);

	void OnScroll(wxScrollWinEvent& event);

	void UpdateVirtualSize();

	wxRect UnscaledToScaled(const wxRect& rect) const;

	wxRect ScaledToUnscaled(const wxRect& rect) const;

	int ClientToPage(const wxPoint& clientPos, wxPoint& pagePos);

	// Returns true if a link is found at the specified position. 
	// If performAction is true the link action will be performed
	bool EvaluateLinkTargetPageAtClientPos(const wxPoint& clientPos, bool performAction);

	void CalcVisiblePages();

	void CalcZoomLevel();

	void AddFindResult(const wxPDFViewTextRange& result);

	int FindOnPage(int pageIndex, bool caseSensitive, bool firstSearch, int characterToStartSearchingFrom);

	// Request redraw for specified page
	void RefreshPage(int pageIndex);
};

#endif // PDFVIEW_IMPL_H