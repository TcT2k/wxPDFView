#ifndef PDFVIEW_IMPL_H
#define PDFVIEW_IMPL_H

#include <wx/sharedptr.h>
#include <wx/thread.h>
#include <wx/graphics.h>
#include <set>
#include <vector>
#include <istream>

#include "fpdf_dataavail.h"
#include "fpdfview.h"
#include "fpdfdoc.h"
#include "fpdfformfill.h"

#include "PDFView.h"

class wxPDFViewImpl: public wxThreadHelper
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

	void SetZoomType(wxPDFViewZoomType zoomType) { m_zoomType = zoomType; };

	wxPDFViewZoomType GetZoomType() const { return m_zoomType; };

	bool LoadStream(wxSharedPtr<std::istream> pStream);

	void CloseDocument();

	std::istream* GetStream() const { return m_pDataStream.get(); };

	void* GetDocument() const { return m_pdfDoc; };

protected:
   virtual wxThread::ExitCode Entry();

private:
	wxPDFView* m_ctrl;

	// Data source
	wxSharedPtr<std::istream> m_pDataStream;
	FPDF_DOCUMENT m_pdfDoc;
	FPDF_FORMHANDLE m_pdfForm;
	FPDF_AVAIL m_pdfAvail;
	wxPDFViewBitmapCache m_bitmapCache;

	// PDF SDK Structures
	FPDF_FILEACCESS m_pdfFileAccess;
	FX_FILEAVAIL m_pdfFileAvail;

	// Document information
	int m_pageCount;
	std::vector<wxRect> m_pageRects;
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
	wxCursor m_handCursor;

	// Bitmap request stores page index of required page bitmaps
	bool m_bmpRequestHandlerActive;
	wxCriticalSection m_bmpRequestCS;
	std::set<int> m_bmpRequest;
	wxCondition* m_bmpRequestHandlerCondition;

	void Init();

	void UpdateDocumentInfo();

	void AlignPageRects();

	bool DrawPage(wxGraphicsContext& gc, int pageIndex, const wxRect& pageRect);

	void RenderPage(int pageIndex);

	void OnCacheBmpAvailable(wxThreadEvent& event);

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

	int GetLinkTargetPageAtClientPos(const wxPoint& clientPos);
};

#endif // PDFVIEW_IMPL_H