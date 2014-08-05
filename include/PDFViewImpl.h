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

class wxPDFViewImpl
{
public:
	wxPDFViewImpl(wxPDFView* ctrl);

	~wxPDFViewImpl();

	void NavigateToPage(wxPDFViewPageNavigation pageNavigation);

	int GetPageCount() const { return m_pageCount; };

	void SetCurrentPage(int pageIndex);

	int GetCurrentPage() const;

	void SetZoom(int zoom);

	int GetZoom() const { return m_zoom; };

	int GetMaxZoom() const { return m_maxZoom; };

	int GetMinZoom() const { return m_minZoom; };

	void SetZoomType(wxPDFViewZoomType zoomType) { m_zoomType = zoomType; };

	wxPDFViewZoomType GetZoomType() const { return m_zoomType; };

	bool LoadStream(wxSharedPtr<std::istream> pStream);

	void CloseDocument();

	void HandleScrollWindow(int dx, int dy);

	std::istream* GetStream() const { return m_pDataStream.get(); };

	void* GetDocument() const { return m_pdfDoc; };

private:
	wxPDFView* m_ctrl;

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
	int m_zoom;
	int m_maxZoom;
	int m_minZoom;
	int m_currentPage;
	int m_firstVisiblePage;
	int m_lastVisiblePage;
	wxCursor m_handCursor;

	void Init();

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
};

#endif // PDFVIEW_IMPL_H