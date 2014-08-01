#include "PDFViewThumbnailListBox.h"

wxPDFViewThumbnailListBox::wxPDFViewThumbnailListBox(wxWindow *parent,
			   wxWindowID id,
			   const wxPoint& pos,
			   const wxSize& size,
			   long style,
			   const wxString& name):
	wxVListBox(parent, id, pos, size, style, name)
{
	Init();
}

void wxPDFViewThumbnailListBox::SetPDFView(wxPDFView* pdfView)
{
	if (m_pdfView != NULL)
	{
		// Disconnect events
		m_pdfView->Unbind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewThumbnailListBox::OnPDFDocumentReady, this);
		m_pdfView->Unbind(wxEVT_PDFVIEW_PAGE_CHANGED, &wxPDFViewThumbnailListBox::OnPDFPageChanged, this);
	}

	m_pdfView = pdfView;

	if (m_pdfView != NULL)
	{
		// Connect events
		m_pdfView->Bind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewThumbnailListBox::OnPDFDocumentReady, this);
		m_pdfView->Bind(wxEVT_PDFVIEW_PAGE_CHANGED, &wxPDFViewThumbnailListBox::OnPDFPageChanged, this);
	}
}

void wxPDFViewThumbnailListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
	// Draw page number
	wxColour textColour;

	if (!IsEnabled())
		textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
	else if (IsSelected(n) && wxWindow::FindFocus() == const_cast<wxPDFViewThumbnailListBox*>(this))
	{
#if wxOSX_USE_COCOA_OR_CARBON
		textColour = *wxWHITE;
#else
		textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT);
#endif
	}
	else
		textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
		
	dc.SetTextForeground(textColour);

	wxRect labelRect = rect.Inflate(-1, -1);
	dc.DrawLabel(wxString::Format("%d", n + 1), labelRect, wxALIGN_BOTTOM | wxALIGN_CENTER);
}

wxCoord wxPDFViewThumbnailListBox::OnMeasureItem(size_t n) const
{
	return 64;
}

void wxPDFViewThumbnailListBox::Init()
{
	m_pdfView = NULL;
	Bind(wxEVT_COMMAND_LISTBOX_SELECTED, &wxPDFViewThumbnailListBox::OnSelectionChanged, this);
}

void wxPDFViewThumbnailListBox::OnPDFDocumentReady(wxCommandEvent& event)
{
	SetItemCount(m_pdfView->GetPageCount());

	event.Skip();
}

void wxPDFViewThumbnailListBox::OnPDFPageChanged(wxCommandEvent& event)
{
	int pageIndex = event.GetInt();
	SetSelection(pageIndex);
	if (!IsVisible(pageIndex))
		ScrollToRow(pageIndex);

	event.Skip();
}

void wxPDFViewThumbnailListBox::OnSelectionChanged(wxCommandEvent& event)
{
	m_pdfView->GotoPage(GetSelection());
	event.Skip();
}
