/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewThumbnailListBoxImpl.cpp
// Purpose:     wxPDFViewThumbnailListBoxImpl implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "private/PDFViewThumbnailListBoxImpl.h"
#include "private/PDFViewImpl.h"

#include <wx/graphics.h>

wxPDFViewThumbnailListBoxImpl::wxPDFViewThumbnailListBoxImpl(wxPDFViewThumbnailListBox* ctrl):
	m_ctrl(ctrl)
{
	m_pdfView = NULL;
	m_ctrl->Bind(wxEVT_COMMAND_LISTBOX_SELECTED, &wxPDFViewThumbnailListBoxImpl::OnSelectionChanged, this);
}

void wxPDFViewThumbnailListBoxImpl::SetPDFView(wxPDFView* pdfView)
{
	if (m_pdfView != NULL)
	{
		// Disconnect events
		m_pdfView->Unbind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewThumbnailListBoxImpl::OnPDFDocumentReady, this);
		m_pdfView->Unbind(wxEVT_PDFVIEW_PAGE_CHANGED, &wxPDFViewThumbnailListBoxImpl::OnPDFPageChanged, this);
		SetPages(NULL);
	}

	m_pdfView = pdfView;

	if (m_pdfView != NULL)
	{
		SetPages(m_pdfView->GetImpl()->GetPages());
		// Connect events
		m_pdfView->Bind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewThumbnailListBoxImpl::OnPDFDocumentReady, this);
		m_pdfView->Bind(wxEVT_PDFVIEW_PAGE_CHANGED, &wxPDFViewThumbnailListBoxImpl::OnPDFPageChanged, this);
	}
}

void wxPDFViewThumbnailListBoxImpl::DrawPage(wxDC& dc, const wxRect& rect, int pageIndex)
{
	if (m_pPages && m_pPages->empty())
		return;

	wxRect pageDrawRect = rect;
	wxPDFViewPage& page = (*m_pPages)[pageIndex];
	wxRect pageRect(m_pdfView->GetImpl()->GetPageSize(pageIndex));
	if (pageRect.height >= pageRect.width)
	{
		pageDrawRect.width = pageDrawRect.height / ((double) pageRect.height / pageRect.width);
	} else {
		pageDrawRect.height = pageDrawRect.width / ((double) pageRect.width / pageRect.height);
	}

	page.DrawThumbnail(this, dc, pageDrawRect.CenterIn(rect));
}

void wxPDFViewThumbnailListBoxImpl::HandleScrollWindow(int WXUNUSED(dx), int WXUNUSED(dy))
{
	UpdateVisiblePages();
}

void wxPDFViewThumbnailListBoxImpl::OnPDFDocumentReady(wxCommandEvent& event)
{
	m_ctrl->SetItemCount(m_pdfView->GetPageCount());
	UpdateVisiblePages();
	m_ctrl->Refresh();

	event.Skip();
}

void wxPDFViewThumbnailListBoxImpl::OnPDFPageChanged(wxCommandEvent& event)
{
	int pageIndex = event.GetInt();
	m_ctrl->SetSelection(pageIndex);
	if (!m_ctrl->IsVisible(pageIndex))
		m_ctrl->ScrollToRow(pageIndex);

	event.Skip();
}

void wxPDFViewThumbnailListBoxImpl::OnSelectionChanged(wxCommandEvent& event)
{
	m_pdfView->GoToPage(m_ctrl->GetSelection());
	UpdateVisiblePages();
	event.Skip();
}

void wxPDFViewThumbnailListBoxImpl::UpdateVisiblePages()
{
	SetVisiblePages(m_ctrl->GetVisibleBegin(), m_ctrl->GetVisibleEnd());
}
