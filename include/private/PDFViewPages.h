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
#include "fpdfdoc.h"

#ifdef __WXMSW__
#define wxPDFVIEW_USE_RENDER_TO_DC
#endif

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

	static wxBitmap CreateBitmap(FPDF_PAGE page, const wxSize& bmpSize, int flags);
};

class wxPDFViewPages: public wxVector<wxPDFViewPage>, public wxEvtHandler, public wxThreadHelper
{
public:
	wxPDFViewPages();

	~wxPDFViewPages();

	void SetDocument(FPDF_DOCUMENT doc);

	void RegisterClient(wxPDFViewPagesClient* client);

	void UnregisterClient(wxPDFViewPagesClient* client);

	void RequestBitmapUpdate();

	FPDF_DOCUMENT doc() const { return m_doc; };

protected:
   virtual wxThread::ExitCode Entry();

private:
	FPDF_DOCUMENT m_doc;
	wxVector<wxPDFViewPagesClient*> m_clients;

	bool m_bmpUpdateHandlerActive;
	wxCondition* m_bmpUpdateHandlerCondition;

	void NotifyPageUpdate(wxPDFViewPagesClient* client, int pageIndex);
};

class wxPDFViewPageBitmapCacheEntry
{
public:
	wxSize requiredSize;
	wxBitmap bitmap;
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

	virtual void OnPageUpdated(int pageIndex) = 0;

private:
	int m_firstVisiblePage;
	int m_lastVisiblePage;
	std::map<int, wxPDFViewPageBitmapCacheEntry> m_bitmapCache;

	friend class wxPDFViewPages;
};

#endif // PDFVIEW_PAGES_H
