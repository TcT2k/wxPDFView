#include "PDFViewBitmapCache.h"

#include <wx/rawbmp.h>

#include "fpdf_ext.h"
#include "fpdfformfill.h"
#include "fpdftext.h"

wxPDFViewBitmapCache::wxPDFViewBitmapCache():
	m_maxCapacity(30)
{

}

bool wxPDFViewBitmapCache::GetBitmapForPage(int pageIndex, const wxSize& size, wxBitmap& bmp)
{
	for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it)
	{
		if (it->first == pageIndex)
		{
			bmp = it->second;
			return bmp.GetSize() == size;
		}
	}

	return false;
}

void wxPDFViewBitmapCache::RenderPage(int pageIndex, const wxSize& bmpSize, void* pdfDoc, void* pdfForm)
{
	FPDF_PAGE page = FPDF_LoadPage(pdfDoc, pageIndex);
	FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page);
	FORM_OnAfterLoadPage(page, pdfForm);
	FORM_DoPageAAction(page, pdfForm, FPDFPAGE_AACTION_OPEN);

	FPDF_BITMAP bitmap = FPDFBitmap_Create(bmpSize.x, bmpSize.y, 0);
	FPDFBitmap_FillRect(bitmap, 0, 0, bmpSize.x, bmpSize.y, 0xFFFFFFFF);

#ifdef __WXMSW__
	wxBitmap bmp(bmpSize, 24);
	wxMemoryDC memDC(bmp);
	memDC.SetBackground(*wxWHITE_BRUSH);
	memDC.Clear();
	FPDF_RenderPage(memDC.GetHDC(), page, 0, 0, bmpSize.x, bmpSize.y, 0, 2);
#else
	FPDF_RenderPageBitmap(bitmap, page, 0, 0, bmpSize.x, bmpSize.y, 0, 2);
	FPDF_FFLDraw(pdfForm, bitmap, page, 0, 0, bmpSize.x, bmpSize.y, 0, 0);
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

	FORM_DoPageAAction(page, pdfForm, FPDFPAGE_AACTION_CLOSE);
	FORM_OnBeforeClosePage(page, pdfForm);
	FPDFText_ClosePage(text_page);
	FPDF_ClosePage(page);

	// Put rendered bitmap in cache
	if (m_entries.size() >= m_maxCapacity)
	{
		wxLogDebug("Removing bitmap cache entry...");
		m_entries.pop_front();
	}

	bool newPage = true;

	for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
	{
		if (it->first == pageIndex)
		{
			it->second = bmp;
			newPage = false;
			break;
		}
	}

	if (newPage)
		m_entries.push_back(std::make_pair<int, wxBitmap>(pageIndex, bmp));
}
