/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFView.cpp
// Purpose:     wxPDFView implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "PDFView.h"
#include "private/PDFViewImpl.h"

#include <fstream>

wxDEFINE_EVENT(wxEVT_PDFVIEW_DOCUMENT_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_DOCUMENT_CLOSED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_PAGE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_ZOOM_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_ZOOM_TYPE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_URL_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_REMOTE_GOTO, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_LAUNCH, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_UNSUPPORTED_FEATURE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_ACTIVITY, wxCommandEvent);

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
		SetBackgroundColour(wxColour(86, 86, 86));
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

void wxPDFView::GoToPage(int pageIndex, const wxRect* centerRect)
{
	if (m_impl)
		m_impl->GoToPage(pageIndex, centerRect);
}

int wxPDFView::GetMostVisiblePage() const
{
	if (m_impl)
		return m_impl->GetMostVisiblePage();
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

long wxPDFView::Find(const wxString& text, int flags)
{
	if (m_impl)
		return m_impl->Find(text, flags);
	else
		return wxNOT_FOUND;
}

bool wxPDFView::IsPrintAllowed() const
{
	if (m_impl)
		return m_impl->IsPrintAllowed();
	else
		return false;
}

wxPrintout* wxPDFView::CreatePrintout() const
{
	if (m_impl)
		return m_impl->CreatePrintout();
	else
		return NULL;
}

wxPrintDialogData wxPDFView::GetPrintDialogData() const
{
	if (m_impl)
		return m_impl->GetPrintDialogData();
	else
		return wxPrintDialogData();
}

const wxString& wxPDFView::GetDocumentTitle() const
{
	static wxString empty;
	if (m_impl)
		return m_impl->GetDocumentTitle();
	else
		return empty;
}

bool wxPDFView::LoadFile(const wxString& fileName, const wxString& password)
{
	wxSharedPtr<std::istream> pStr(new std::ifstream((const char*) fileName.c_str(), std::ios::in | std::ios::binary));
	return LoadStream(pStr, password);
}

bool wxPDFView::LoadStream(wxSharedPtr<std::istream> pStream, const wxString& password)
{
	if (m_impl)
	{
		return m_impl->LoadStream(pStream, password);
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

void wxPDFView::SetPrintValidator(wxPDFViewPrintValidator* validator)
{
	m_impl->SetPrintValidator(validator);
}

wxPDFViewPrintValidator* wxPDFView::GetPrintValidator() const
{
	if (m_impl)
		return m_impl->GetPrintValidator();
	else
		return NULL;
}

