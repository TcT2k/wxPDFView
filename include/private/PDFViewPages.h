/////////////////////////////////////////////////////////////////////////////
// Name:        include/private/PDFViewPages.h
// Purpose:     wxPDFViewPages class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_PAGES_H
#define PDFVIEW_PAGES_H

#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/thread.h>
#include <vector>
#include <map>
#include "fpdfdoc.h"

#ifdef __WXMSW__
#define wxPDFVIEW_USE_RENDER_TO_DC
#endif

class wxPDFViewPages;

class wxPDFViewPage
{
public:
	wxPDFViewPage(wxPDFViewPages* pages, int index);

	~wxPDFViewPage();

	int GetIndex() const { return m_index; };

	void Unload();

	FPDF_PAGE GetPage();

	FPDF_TEXTPAGE GetTextPage();

	void Draw(wxDC& dc, wxGraphicsContext& gc, const wxRect& rect);

	void DrawThumbnail(wxDC& dc, const wxRect& rect);

	void DrawPrint(wxDC& dc, const wxRect& rect);

	bool IsBitmapUpdateRequired() const;

	bool UpdateBitmap();

	wxRect PageToScreen(const wxRect& pageRect, double left, double top, double right, double bottom);

private:
	wxPDFViewPages* m_pages;
	int m_index;
	FPDF_PAGE m_page;
	FPDF_TEXTPAGE m_textPage;
	wxBitmap m_bmp;
	wxSize m_requiredBmpSize;

	void CheckBitmap(const wxSize& bmpSize);

#ifndef wxPDFVIEW_USE_RENDER_TO_DC
	static wxBitmap CreateBitmap(FPDF_PAGE page, const wxSize& bmpSize, int flags);
#endif
};

class wxPDFViewPages: public std::vector<wxPDFViewPage>, public wxEvtHandler, public wxThreadHelper
{
public:
	wxPDFViewPages();

	~wxPDFViewPages();

	void SetDocument(FPDF_DOCUMENT doc);

	void SetVisiblePages(int firstPage, int lastPage);

	bool IsPageVisible(int pageIndex) const;

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
};

wxDECLARE_EVENT(wxEVT_PDFVIEW_PAGE_UPDATED, wxThreadEvent);

#endif // PDFVIEW_PAGES_H
