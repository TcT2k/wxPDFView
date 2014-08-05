#include "private/PDFViewPages.h"

#include <algorithm>

#include <wx/rawbmp.h>

wxDEFINE_EVENT(wxEVT_PDFVIEW_PAGE_UPDATED, wxThreadEvent);

//
// wxPDFViewPage
// 

wxPDFViewPage::wxPDFViewPage(wxPDFViewPages* pages, int index):
	m_pages(pages),
	m_index(index),
	m_page(NULL)
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
		m_page = NULL;
	}
	m_bmp = wxNullBitmap;
}

FPDF_PAGE wxPDFViewPage::GetPage()
{
	if (!m_page)
	{
		m_page = FPDF_LoadPage(m_pages->doc(), m_index);
	}
	return m_page;
}

void wxPDFViewPage::Draw(wxDC& dc, wxGraphicsContext& gc, const wxRect& rect)
{
	// Draw page background
	wxRect bgRect = rect.Inflate(2, 2);
	gc.SetBrush(*wxWHITE_BRUSH);
	gc.SetPen(*wxBLACK_PEN);
	gc.DrawRectangle(bgRect.x, bgRect.y, bgRect.width, bgRect.height);

	// Draw any size bitmap regardless of size (blurry page is better than empty page)
	if (m_bmp.IsOk())
		gc.DrawBitmap(m_bmp, rect.x, rect.y, rect.width, rect.height);

	// Calculate the required bitmap size
	wxSize bmpSize = rect.GetSize();
	double scaleX, scaleY;
	dc.GetUserScale(&scaleX, &scaleY);
	bmpSize.x *= scaleX;
	bmpSize.y *= scaleY;

	// Request new bitmap if no bitmap is available or it has the wrong size
	if (!m_bmp.IsOk() || m_bmp.GetSize() != bmpSize)
	{
		m_requiredBmpSize = bmpSize;
		m_pages->RequestBitmapUpdate();
	}
}

bool wxPDFViewPage::IsBitmapUpdateRequired() const
{
	return !m_bmp.IsOk() || m_bmp.GetSize() != m_requiredBmpSize;
}

void wxPDFViewPage::UpdateBitmap()
{
	// Check if bitmap has to be rerendered
	wxSize bmpSize = m_requiredBmpSize;

	// Render to bitmap
	FPDF_PAGE page = GetPage();

#ifdef __WXMSW__
	wxBitmap bmp(bmpSize, 24);
	wxMemoryDC memDC(bmp);
	memDC.SetBackground(*wxWHITE_BRUSH);
	memDC.Clear();
	FPDF_RenderPage(memDC.GetHDC(), page, 0, 0, bmpSize.x, bmpSize.y, 0, 2);
#else
	FPDF_BITMAP bitmap = FPDFBitmap_Create(bmpSize.x, bmpSize.y, 0);
	FPDFBitmap_FillRect(bitmap, 0, 0, bmpSize.x, bmpSize.y, 0xFFFFFFFF);

	FPDF_RenderPageBitmap(bitmap, page, 0, 0, bmpSize.x, bmpSize.y, 0, 2);
	unsigned char* buffer =
		reinterpret_cast<unsigned char*>(FPDFBitmap_GetBuffer(bitmap));

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
#endif

	m_bmp = bmp;
}

//
// wxPDFViewPages
//

wxPDFViewPages::wxPDFViewPages():
	m_doc(NULL)
{
	m_firstVisiblePage = -1;
	m_lastVisiblePage = -1;

	// Init bitmap request handler
	m_bmpUpdateHandlerActive = true;
	m_bmpUpdateHandlerCondition = NULL;

	if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)     
	{         
		wxLogError("Could not create the worker thread!");         
		return;     
	}

	if (GetThread()->Run() != wxTHREAD_NO_ERROR)
	{
		wxLogError("Could not run the worker thread!");
		return;
	}
}

wxPDFViewPages::~wxPDFViewPages()
{
	if (m_bmpUpdateHandlerCondition)
	{
		// Finish bitmap update handler
		m_bmpUpdateHandlerActive = false;
		m_bmpUpdateHandlerCondition->Signal();
		GetThread()->Wait();
	}
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

void wxPDFViewPages::SetVisiblePages(int firstPage, int lastPage)
{
	UnloadPages(0, firstPage - 1);
	UnloadPages(lastPage + 1, size() - 1);
	m_firstVisiblePage = firstPage;
	m_lastVisiblePage = lastPage;
}

void wxPDFViewPages::UnloadPages(int firstPage, int lastPage)
{
	// Never unload a pages while a page might be rendering
	wxCriticalSectionLocker csl(m_bmpRenderCS);
	for (int pageIndex = firstPage; pageIndex <= lastPage; ++pageIndex)
	{
		(*this)[pageIndex].Unload();
	}
}

void wxPDFViewPages::RequestBitmapUpdate()
{
	// Notify render thread
	m_bmpUpdateHandlerCondition->Broadcast();
}

wxThread::ExitCode wxPDFViewPages::Entry()
{
	wxMutex requestHandlerMutex;
	requestHandlerMutex.Lock();
	m_bmpUpdateHandlerCondition = new wxCondition(requestHandlerMutex);

	while (m_bmpUpdateHandlerActive)
	{
		m_bmpUpdateHandlerCondition->Wait();

		for (int pageIndex = m_firstVisiblePage; pageIndex <= m_lastVisiblePage && m_bmpUpdateHandlerActive; ++pageIndex)
		{
			wxLogDebug("Rendering page %d...", pageIndex);
			wxPDFViewPage& page = (*this)[pageIndex];

			if (page.IsBitmapUpdateRequired())
			{
				wxCriticalSectionLocker csl(m_bmpRenderCS);
				page.UpdateBitmap();
			}

			wxThreadEvent evt(wxEVT_PDFVIEW_PAGE_UPDATED);
			evt.SetInt(pageIndex);
			AddPendingEvent(evt);
		}
	}

	delete m_bmpUpdateHandlerCondition;
	m_bmpUpdateHandlerCondition = NULL;

	return 0;
}
