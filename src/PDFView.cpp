#include "PDFView.h"
#include "private/PDFViewImpl.h"

#include <fstream>

wxDEFINE_EVENT(wxEVT_PDFVIEW_DOCUMENT_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_PAGE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_ZOOM_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_ZOOM_TYPE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_URL_CLICKED, wxCommandEvent);

//
// wxPDFView
//

wxPDFView::wxPDFView(wxWindow *parent,
	wxWindowID winid,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name)
{
	Create(parent, winid, pos, size, style, name);
}

wxPDFView::~wxPDFView()
{
	delete m_impl;
}

bool wxPDFView::Create(wxWindow *parent,
	wxWindowID winid,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name)
{
	bool res = wxScrolledCanvas::Create(parent, winid, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name);
	if (res)
	{
		m_impl = new wxPDFViewImpl(this);

		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetBackgroundColour(*wxLIGHT_GREY);
	}
	return res;
}

void wxPDFView::NavigateToPage(wxPDFViewPageNavigation pageNavigation)
{
	if (m_impl)
		m_impl->NavigateToPage(pageNavigation);
}

int wxPDFView::GetPageCount() const
{
	if (m_impl)
		return m_impl->GetPageCount();
	else
		return 0;
}

const wxPDFViewBookmark* wxPDFView::GetRootBookmark() const
{
	if (m_impl)
		return m_impl->GetRootBookmark();
	else
		return 0;
}

void wxPDFView::SetCurrentPage(int pageIndex)
{
	if (m_impl)
		m_impl->SetCurrentPage(pageIndex);
}

int wxPDFView::GetCurrentPage() const
{
	if (m_impl)
		return m_impl->GetCurrentPage();
	else
		return 0;
}

void wxPDFView::SetZoom(double zoom)
{
	if (m_impl)
	{
		m_impl->SetZoomType(wxPDFVIEW_ZOOM_TYPE_FREE);
		m_impl->SetZoom(zoom);
	}
}

double wxPDFView::GetZoom() const
{
	if (m_impl)
		return m_impl->GetZoom();
	else
		return 0;
}

double wxPDFView::GetMaxZoom() const
{
	if (m_impl)
		return m_impl->GetMaxZoom();
	else
		return 0;
}

double wxPDFView::GetMinZoom() const
{
	if (m_impl)
		return m_impl->GetMinZoom();
	else
		return 0;
}

void wxPDFView::SetZoomType(wxPDFViewZoomType zoomType)
{
	if (m_impl)
		m_impl->SetZoomType(zoomType);
}

wxPDFViewZoomType wxPDFView::GetZoomType() const
{
	if (m_impl)
		return m_impl->GetZoomType();
	else
		return wxPDFVIEW_ZOOM_TYPE_FREE;
}

bool wxPDFView::LoadFile(const wxString& fileName)
{
	wxSharedPtr<std::istream> pStr(new std::ifstream((const char*) fileName.c_str(), std::ios::in | std::ios::binary));
	return LoadStream(pStr);
}

bool wxPDFView::LoadStream(wxSharedPtr<std::istream> pStream)
{
	if (m_impl)
	{
		return m_impl->LoadStream(pStream);
	} else
		return false;
}

void wxPDFView::CloseDocument()
{
	if (m_impl)
		m_impl->CloseDocument();
}

void wxPDFView::ScrollWindow(int dx, int dy, const wxRect *rect)
{
	wxScrolledCanvas::ScrollWindow(dx, dy, rect);

	if (m_impl)
		m_impl->HandleScrollWindow(dx, dy);
}

