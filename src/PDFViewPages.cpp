/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewPages.cpp
// Purpose:     wxPDFViewPages implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "private/PDFViewPages.h"

#include <wx/rawbmp.h>

#include "fpdf_text.h"

#include <algorithm>

//
// wxPDFViewPage
// 

wxPDFViewPage::wxPDFViewPage(wxPDFViewPages* pages, int index):
	m_pages(pages),
	m_index(index),
	m_page(NULL),
	m_textPage(NULL)
{

}

wxPDFViewPage::~wxPDFViewPage()
{
	Unload();
}

void wxPDFViewPage::Unload()
{
	if (m_page)
	{
		FPDF_ClosePage(m_page);
		if (m_pages->form())
			FORM_OnBeforeClosePage(m_page, m_pages->form());
		m_page = NULL;
	}
	if (m_textPage)
	{
		FPDFText_ClosePage(m_textPage);
		m_textPage = NULL;
	}
}

FPDF_PAGE wxPDFViewPage::GetPage()
{
	if (!m_page)
	{
		m_page = FPDF_LoadPage(m_pages->doc(), m_index);
		if (m_pages->form())
			FORM_OnAfterLoadPage(m_page, m_pages->form());
	}
	return m_page;
}

FPDF_TEXTPAGE wxPDFViewPage::GetTextPage()
{
	if (!m_textPage)
	{
		m_textPage = FPDFText_LoadPage(GetPage());
	}

	return m_textPage;
}

wxRect wxPDFViewPage::PageToScreen(const wxRect& pageRect, double left, double top, double right, double bottom)
{

	int screenLeft, screenTop, screenRight, screenBottom;
	FPDF_PageToDevice(GetPage(), 0, 0, pageRect.width, pageRect.height, 0, left, top, &screenLeft, &screenTop);
	FPDF_PageToDevice(GetPage(), 0, 0, pageRect.width, pageRect.height, 0, right, bottom, &screenRight, &screenBottom);

	return wxRect(screenLeft, screenTop, screenRight - screenLeft + 1, screenBottom - screenTop + 1);
}

void wxPDFViewPage::Draw(wxPDFViewPagesClient* client, wxDC& dc, wxGraphicsContext& gc, const wxRect& rect)
{
	// Calculate the required bitmap size
	wxSize bmpSize = rect.GetSize();
	double scaleX, scaleY;
	dc.GetUserScale(&scaleX, &scaleY);
	bmpSize.x *= scaleX;
	bmpSize.y *= scaleY;

	double screenScale = dc.GetContentScaleFactor();
	bmpSize.x *= screenScale;
	bmpSize.y *= screenScale;

	wxBitmap bmp = client->GetCachedBitmap(m_index, bmpSize);
	if (bmp.IsOk())
	{
		gc.DrawBitmap(bmp, rect.x, rect.y, rect.width, rect.height);
	} else {
		// Draw empty page
		gc.SetBrush(*wxWHITE_BRUSH);
		gc.SetPen(wxNullPen);
		gc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
	}
}

void wxPDFViewPage::DrawThumbnail(wxPDFViewPagesClient* client, wxDC& dc, const wxRect& rect)
{
	dc.SetBackground(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*wxLIGHT_GREY_PEN);
	dc.DrawRectangle(rect.Inflate(1, 1));

	wxBitmap bmp = client->GetCachedBitmap(m_index, rect.GetSize());
	if (bmp.Ok())
	{
		dc.DrawBitmap(bmp, rect.GetPosition());
	}
}

void wxPDFViewPage::DrawPrint(wxDC& dc, const wxRect& rect, bool forceBitmap)
{
	FPDF_PAGE page = GetPage();

	int renderFlags = FPDF_ANNOT | FPDF_PRINTING;
#ifdef wxPDFVIEW_USE_RENDER_TO_DC
	if (!forceBitmap)
	{
		FPDF_RenderPage(dc.GetHDC(), page, rect.x, rect.y, rect.width, rect.height, 0, renderFlags);
	} else
#endif
	{
		wxBitmap bmp = CreateBitmap(page, m_pages->form(), rect.GetSize(), renderFlags);
		dc.DrawBitmap(bmp, wxPoint(0, 0));
	}
}

wxBitmap wxPDFViewPage::CreateCacheBitmap(const wxSize& bmpSize)
{
	// Render to bitmap
	FPDF_PAGE page = GetPage();

#ifdef wxPDFVIEW_USE_RENDER_TO_DC
	wxBitmap bmp(bmpSize, 24);
	wxMemoryDC memDC(bmp);
	memDC.SetBackground(*wxWHITE_BRUSH);
	memDC.Clear();
	FPDF_RenderPage(memDC.GetHDC(), page, 0, 0, bmpSize.x, bmpSize.y, 0, FPDF_LCD_TEXT);
	return bmp;
#else
	return CreateBitmap(page, m_pages->form(), bmpSize, FPDF_LCD_TEXT);
#endif
}

wxBitmap wxPDFViewPage::CreateBitmap(FPDF_PAGE page, FPDF_FORMHANDLE form, const wxSize& bmpSize, int flags)
{
	FPDF_BITMAP bitmap = FPDFBitmap_Create(bmpSize.x, bmpSize.y, 0);
	FPDFBitmap_FillRect(bitmap, 0, 0, bmpSize.x, bmpSize.y, 0xFFFFFFFF);

	FPDF_RenderPageBitmap(bitmap, page, 0, 0, bmpSize.x, bmpSize.y, 0, flags);
	unsigned char* buffer =
		reinterpret_cast<unsigned char*>(FPDFBitmap_GetBuffer(bitmap));

	if (form)
		FPDF_FFLDraw(form, bitmap, page, 0, 0, bmpSize.x, bmpSize.y, 0, flags);

	// Convert BGRA image data from PDF SDK to RGB image data
	wxBitmap bmp(bmpSize, 24);
	unsigned char* srcP = buffer;
	wxNativePixelData data(bmp);
	wxNativePixelData::Iterator p(data);
	for (int y = 0; y < bmpSize.y; ++y)
	{
		wxNativePixelData::Iterator rowStart = p;

		for (int x = 0; x < bmpSize.x; ++x, ++p)
		{
			p.Blue() = *(srcP++);
			p.Green() = *(srcP++);
			p.Red() = *(srcP++);
			srcP++;
		}

		p = rowStart;
		p.OffsetY(data, 1);
	}

	FPDFBitmap_Destroy(bitmap);

	return bmp;
}

//
// wxPDFViewPages
//

wxPDFViewPages::wxPDFViewPages():
	m_doc(NULL)
{
}

wxPDFViewPages::~wxPDFViewPages()
{
}

void wxPDFViewPages::SetDocument(FPDF_DOCUMENT doc)
{
	m_doc = doc;
	clear();
	if (!doc)
		return;

	int pageCount = FPDF_GetPageCount(m_doc);
	reserve(pageCount);
	for (int i = 0; i < pageCount; ++i)
		push_back(wxPDFViewPage(this, i));
}

void wxPDFViewPages::RegisterClient(wxPDFViewPagesClient* client)
{
	m_clients.push_back(client);
}

void wxPDFViewPages::UnregisterClient(wxPDFViewPagesClient* client)
{
	std::remove(m_clients.begin(), m_clients.end(), client);
}

void wxPDFViewPages::UnloadInvisiblePages()
{
	for (int pageIndex = 0; pageIndex < (int) size(); ++pageIndex)
	{
		int pageVisible = false;
		for (wxVector<wxPDFViewPagesClient*>::iterator client = m_clients.begin(); client != m_clients.end(); ++client)
		{
			if ((*client)->IsPageVisible(pageIndex))
			{
				pageVisible = true;
			} else
				(*client)->RemoveCachedBitmap(pageIndex);
		}

		if (!pageVisible)
			(*this)[pageIndex].Unload();
	}
}

//
// wxPDFViewPageClient
//

wxPDFViewPagesClient::wxPDFViewPagesClient()
{
	m_pPages = NULL;
	m_firstVisiblePage = -1;
	m_lastVisiblePage = -1;
}

void wxPDFViewPagesClient::SetPages(wxPDFViewPages* pages)
{
	if (m_pPages)
		m_pPages->UnregisterClient(this);
	m_pPages = pages;
	if (m_pPages)
		m_pPages->RegisterClient(this);
}

void wxPDFViewPagesClient::SetVisiblePages(int firstPage, int lastPage)
{
	if (!m_pPages)
		return;

	if (firstPage < 0)
		firstPage = 0;
	if (lastPage >= (int) m_pPages->size())
		lastPage = m_pPages->size() - 1;

	m_firstVisiblePage = firstPage;
	m_lastVisiblePage = lastPage;

	m_pPages->UnloadInvisiblePages();
}

bool wxPDFViewPagesClient::IsPageVisible(int pageIndex) const
{
	return pageIndex >= m_firstVisiblePage && pageIndex <= m_lastVisiblePage;
}

wxBitmap wxPDFViewPagesClient::GetCachedBitmap(int pageIndex, const wxSize& size)
{
	wxBitmap bmp = m_bitmapCache[pageIndex];
	if (!bmp.IsOk() || bmp.GetSize() != size)
	{
		bmp = (*m_pPages)[pageIndex].CreateCacheBitmap(size);
		m_bitmapCache[pageIndex] = bmp;
	}

	return bmp;
}

void wxPDFViewPagesClient::RemoveCachedBitmap(int pageIndex)
{
	m_bitmapCache.erase(pageIndex);
}

void wxPDFViewPagesClient::ClearBitmapCache()
{
	m_bitmapCache.clear();
}
