#ifndef PDFVIEW_H
#define PDFVIEW_H

#include <wx/wx.h>
#include <wx/dynarray.h>
#include <wx/thread.h>
#include <wx/graphics.h>
#include <set>
#include <istream>

#include "PDFViewBitmapCache.h"

WX_DECLARE_OBJARRAY(wxSize, ArrayOfSize);

class wxPDFView: public wxScrolledCanvas, public wxThreadHelper
{
public:
	wxPDFView(wxWindow *parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxScrolledWindowStyle,
		const wxString& name = wxPanelNameStr);

	virtual ~wxPDFView();

	void GotoPage(int pageIndex);

	void GotoNextPage();

	void GotoPrevPage();

	int GetCurrentPage() const;

	int GetPageCount() const { return m_pageCount;  };

	void SetZoom(int zoom);

	int GetZoom() const { return m_zoom; };

	int GetMaxZoom() const { return m_maxZoom; };

	int GetMinZoom() const { return m_minZoom; };

	void ZoomFitToPage();

	void ZoomFitPageWidth();

	void LoadFile(const wxString& fileName);

	void LoadStream(std::istream* pStream, bool takeOwnership = false);

	void CloseDocument();

	std::istream* GetStream() const { return m_pDataStream; };

	void* GetDocument() const { return m_pdfDoc; };

protected:
   virtual wxThread::ExitCode Entry();

private:
	// Data source
	bool m_dataStreamOwned;
	std::istream* m_pDataStream;
	void* m_pdfDoc;
	void* m_pdfForm;
	void* m_pdfAvail;
	wxPDFViewBitmapCache m_bitmapCache;

	// PDF SDK Structures
	void* m_pdfFileAccess;
	void* m_pdfFileAvail;

	// Document information
	int m_pageCount;
	ArrayOfSize m_pageSizes;
	int m_docHeight;
	int m_docMaxWidth;

	// Display settings
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

	wxRect GetPageRect(int pageIndex) const;

	bool DrawPage(wxGraphicsContext& gc, int pageIndex, const wxRect& pageRect);

	void RenderPage(int pageIndex);

	void OnCacheBmpAvailable(wxThreadEvent& event);

	void OnPaint(wxPaintEvent& event);

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

wxDECLARE_EVENT(wxEVT_PDFVIEW_DOCUMENT_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_PAGE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_ZOOM_CHANGED, wxCommandEvent);

#endif // PDFVIEW_H
