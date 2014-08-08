#include "private/PDFViewPrintout.h"
#include "private/PDFViewImpl.h"

wxPDFViewPrintout::wxPDFViewPrintout(wxPDFView* pdfView):
	wxPrintout(GetDocTitle(pdfView)),
	m_ctrl(pdfView)
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
            RenderPage(*dc, page - 1);
        return true;
    }
    else 
		return false;
}

bool wxPDFViewPrintout::HasPage(int page)
{
	return page > 0 && page <= m_ctrl->GetImpl()->m_pageCount;
}

void wxPDFViewPrintout::GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo)
{
	*minPage = 1;
	*maxPage = m_ctrl->GetImpl()->m_pageCount - 1;
	*selPageFrom = *minPage;
	*selPageTo = *maxPage;
}

bool wxPDFViewPrintout::OnBeginDocument(int startPage, int endPage)
{
	if (!wxPrintout::OnBeginDocument(startPage, endPage))
		return false;

	return true;
}

void wxPDFViewPrintout::OnPreparePrinting()
{
	wxPrintout::OnPreparePrinting();
}

void wxPDFViewPrintout::RenderPage(wxDC& dc, int pageIndex)
{
	int pageWidth, pageHeight;
    GetPageSizePixels(&pageWidth, &pageHeight);

	// Prepare DC
    dc.SetBackgroundMode(wxTRANSPARENT);

	// Draw page content
	wxRect printRect(0, 0, pageWidth, pageHeight);
	wxPDFViewPage& page = m_ctrl->GetImpl()->m_pages[pageIndex];
	page.DrawPrint(dc, printRect);
	page.Unload();
}
