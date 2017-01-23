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
#include <wx/vector.h>

#include <map>

#include "fpdf_doc.h"
#include "fpdf_formfill.h"

class wxPDFViewPages;
class wxPDFViewPagesClient;

class wxPDFViewPage
{
public:
	wxPDFViewPage(wxPDFViewPages* pages, int index);

	~wxPDFViewPage();

	int GetIndex() const { return m_index; };

	void Unload();

	FPDF_PAGE GetPage();

	FPDF_TEXTPAGE GetTextPage();

	void Draw(wxPDFViewPagesClient* client, wxDC& dc, wxGraphicsContext& gc, const wxRect& rect);

	void DrawThumbnail(wxPDFViewPagesClient* client, wxDC& dc, const wxRect& rect);

	void DrawPrint(wxDC& dc, const wxRect& rect, bool forceBitmap);

	wxBitmap CreateCacheBitmap(const wxSize& bmpSize);

	wxRect PageToScreen(const wxRect& pageRect, double left, double top, double right, double bottom);

private:
	wxPDFViewPages* m_pages;
	int m_index;
	FPDF_PAGE m_page;
	FPDF_TEXTPAGE m_textPage;

	static wxBitmap CreateBitmap(FPDF_PAGE page, FPDF_FORMHANDLE form, const wxSize& bmpSize, int flags);
};

class wxPDFViewPages: public wxVector<wxPDFViewPage>
{
public:
	wxPDFViewPages();

	~wxPDFViewPages();

	void SetDocument(FPDF_DOCUMENT doc);

	void RegisterClient(wxPDFViewPagesClient* client);

	void UnregisterClient(wxPDFViewPagesClient* client);

	void UnloadInvisiblePages();

	FPDF_DOCUMENT doc() const { return m_doc; };

	void SetForm(FPDF_FORMHANDLE form) { m_pdfForm = form; }

	FPDF_FORMHANDLE form() const { return m_pdfForm; }

private:
	FPDF_DOCUMENT m_doc;
	FPDF_FORMHANDLE m_pdfForm;
	wxVector<wxPDFViewPagesClient*> m_clients;
};

class wxPDFViewPagesClient
{
public:
	wxPDFViewPagesClient();

	void SetPages(wxPDFViewPages* pages);

	void SetVisiblePages(int firstPage, int lastPage);

	bool IsPageVisible(int pageIndex) const;

	int GetFirstVisiblePage() const { return m_firstVisiblePage; };

	int GetLastVisiblePage() const { return m_lastVisiblePage; };

	wxBitmap GetCachedBitmap(int pageIndex, const wxSize& size);

	void RemoveCachedBitmap(int pageIndex);

protected:
	wxPDFViewPages* m_pPages;

	void ClearBitmapCache();

private:
	int m_firstVisiblePage;
	int m_lastVisiblePage;
	std::map<int, wxBitmap> m_bitmapCache;

	friend class wxPDFViewPages;
};

#endif // PDFVIEW_PAGES_H
