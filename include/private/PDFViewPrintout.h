#ifndef PDFVIEW_PRINTOUT_H
#define PDFVIEW_PRINTOUT_H

#include <wx/print.h>
class wxPDFView;

class wxPDFViewPrintout: public wxPrintout
{
public:
	wxPDFViewPrintout(wxPDFView* pdfView);

	bool OnPrintPage(int page);
	bool HasPage(int page);
	void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);
	bool OnBeginDocument(int startPage, int endPage);
	void OnPreparePrinting();

private:
	wxPDFView* m_ctrl;

	void RenderPage(wxDC& dc, int pageIndex);

	wxString GetDocTitle(wxPDFView* pdfView);
};


#endif // PDFVIEW_PRINTOUT_H
