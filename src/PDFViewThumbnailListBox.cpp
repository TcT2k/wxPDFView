/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewThumbnailListBox.cpp
// Purpose:     wxPDFViewThumbnailListBox implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "PDFViewThumbnailListBox.h"

#include "private/PDFViewThumbnailListBoxImpl.h"

bool wxPDFViewThumbnailListBox::Create(wxWindow *parent,
			wxWindowID id,
			const wxPoint& pos,
			const wxSize& size,
			long style,
			const wxString& name)
{
	if (!wxVListBox::Create(parent, id, pos, size, style, name))
		return false;

	m_impl = new wxPDFViewThumbnailListBoxImpl(this);

	return true;
}

wxPDFViewThumbnailListBox::~wxPDFViewThumbnailListBox()
{
	if (m_impl)
	{
		m_impl->SetPDFView(NULL);
		wxDELETE(m_impl);
	}
}

void wxPDFViewThumbnailListBox::SetPDFView(wxPDFView* pdfView)
{
	if (m_impl)
		m_impl->SetPDFView(pdfView);
}

wxPDFView* wxPDFViewThumbnailListBox::GetPDFView() const
{
	if (m_impl)
		return m_impl->GetPDFView();
	else
		return NULL;
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
	dc.DrawLabel(wxString::Format("%lu", n + 1), labelRect, wxALIGN_BOTTOM | wxALIGN_CENTER);

	wxRect pageDrawRect = labelRect.Inflate(-1, -1);
	pageDrawRect.height -= dc.GetCharHeight();

	if (m_impl)
		m_impl->DrawPage(dc, pageDrawRect, n);
}

wxCoord wxPDFViewThumbnailListBox::OnMeasureItem(size_t WXUNUSED(n)) const
{
	return wxDLG_UNIT(this, wxSize(-1, 72)).GetHeight();
}

void wxPDFViewThumbnailListBox::ScrollWindow(int dx, int dy, const wxRect *rect)
{
	wxVListBox::ScrollWindow(dx, dy, rect);

	if (m_impl)
		m_impl->HandleScrollWindow(dx, dy);
}
