/////////////////////////////////////////////////////////////////////////////
// Name:        include/private/PDFViewPrintout.h
// Purpose:     wxPDFViewPrintout class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_PRINTOUT_H
#define PDFVIEW_PRINTOUT_H

#include <wx/print.h>

class wxPDFView;

class wxPDFViewPrintout: public wxPrintout
{
public:
	wxPDFViewPrintout(wxPDFView* pdfView, bool forceBitmapPrint);

	bool OnPrintPage(int page);
	bool HasPage(int page);
	void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);
	bool OnBeginDocument(int startPage, int endPage);
	void OnPreparePrinting();

private:
	wxPDFView* m_ctrl;
	bool m_forceBitmapPrint;
	bool m_printValidated;

	void RenderPage(wxDC& dc, int pageIndex);

	wxString GetDocTitle(wxPDFView* pdfView);
};


#endif // PDFVIEW_PRINTOUT_H
