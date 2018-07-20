/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewPrintout.cpp
// Purpose:     wxPDFViewPrintout implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "private/PDFViewPrintout.h"
#include "private/PDFViewImpl.h"

wxPDFViewPrintout::wxPDFViewPrintout(wxPDFView* pdfView, bool forceBitmapPrint):
	wxPrintout(GetDocTitle(pdfView)),
	m_ctrl(pdfView),
	m_forceBitmapPrint(forceBitmapPrint),
	m_printValidated(true)
{

}

wxString wxPDFViewPrintout::GetDocTitle(wxPDFView* pdfView)
{
	if (pdfView->GetDocumentTitle().empty())
		return _("PDF Document");
	else
		return pdfView->GetDocumentTitle();
}

bool wxPDFViewPrintout::OnPrintPage(int page)
{
	wxDC *dc = GetDC();
	if (dc && dc->IsOk())
	{
		if (HasPage(page))
		{
			return RenderPage(*dc, page - 1);
		}
		else
			return false;
	}
	else 
		return false;
}

bool wxPDFViewPrintout::HasPage(int page)
{
	return m_printValidated && page > 0 && page <= m_ctrl->GetImpl()->GetPageCount();
}

void wxPDFViewPrintout::GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo)
{
	*minPage = 1;
	*maxPage = m_ctrl->GetImpl()->GetPageCount();
	*selPageFrom = *minPage;
	*selPageTo = *maxPage;
}

bool wxPDFViewPrintout::OnBeginDocument(int startPage, int endPage)
{
	m_printValidated = false;
	if (!wxPrintout::OnBeginDocument(startPage, endPage))
		return false;
	
	wxPDFViewPrintValidator* validator = m_ctrl->GetPrintValidator();
	if (validator && !validator->OnBeginPrint(startPage, endPage))
		return false;

	m_printValidated = true;
	return true;
}

void wxPDFViewPrintout::OnPreparePrinting()
{
	wxPrintout::OnPreparePrinting();
}

bool wxPDFViewPrintout::RenderPage(wxDC& dc, int pageIndex)
{
	int pageWidth, pageHeight;
	GetPageSizePixels(&pageWidth, &pageHeight);

	// Prepare DC
	dc.SetBackgroundMode(wxTRANSPARENT);

	// Draw page content
	wxRect printRect(0, 0, pageWidth, pageHeight);
	wxPDFViewPage& page = m_ctrl->GetImpl()->m_pages[pageIndex];
	bool result = page.DrawPrint(dc, printRect, m_forceBitmapPrint);
	page.Unload();
	return result;
}
