#ifndef PDFVIEW_PAGES_H
#define PDFVIEW_PAGES_H

#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/thread.h>
#include <vector>
#include <map>
#include "fpdfdoc.h"

class wxPDFViewPages;

class wxPDFViewPage
{
public:
	wxPDFViewPage(wxPDFViewPages* pages, int index);

	~wxPDFViewPage();

	void Unload();

	FPDF_PAGE GetPage();

	void Draw(wxDC& dc, wxGraphicsContext& gc, const wxRect& rect);

	bool IsBitmapUpdateRequired() const;

	void UpdateBitmap();

private:
	wxPDFViewPages* m_pages;
	int m_index;
	FPDF_PAGE m_page;
	wxBitmap m_bmp;
	wxSize m_requiredBmpSize;
};

class wxPDFViewPages: public std::vector<wxPDFViewPage>, public wxEvtHandler, public wxThreadHelper
{
public:
	wxPDFViewPages();

	~wxPDFViewPages();

	void SetDocument(FPDF_DOCUMENT doc);

	void SetVisiblePages(int firstPage, int lastPage);

	void UnloadPages(int firstPage, int lastPage);

	void RequestBitmapUpdate();

	FPDF_DOCUMENT doc() const { return m_doc; };

protected:
   virtual wxThread::ExitCode Entry();

private:
	FPDF_DOCUMENT m_doc;
	int m_firstVisiblePage;
	int m_lastVisiblePage;

	bool m_bmpUpdateHandlerActive;
	wxCondition* m_bmpUpdateHandlerCondition;
	wxCriticalSection m_bmpRenderCS;
};

wxDECLARE_EVENT(wxEVT_PDFVIEW_PAGE_UPDATED, wxThreadEvent);

#endif // PDFVIEW_PAGES_H