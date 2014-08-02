#ifndef PDFVIEW_BITMAP_CACHE_H
#define PDFVIEW_BITMAP_CACHE_H

#include <wx/wx.h>
#include <list>
#include <utility>

class wxPDFViewBitmapCache
{
public:
	wxPDFViewBitmapCache();

	// Finds bitmap for page at any size returns false if other size was found
	bool GetBitmapForPage(int pageIndex, const wxSize& size, wxBitmap& bmp) const;

	void RenderPage(int pageIndex, const wxSize& bmpSize, void* pdfDoc, void* pdfForm);

	// Removes all entries from cache
	void Clear();

private:
	unsigned int m_maxCapacity;
	wxCriticalSection m_cacheCS;
	std::list< std::pair<int, wxBitmap> > m_entries;
};


#endif // PDFVIEW_BITMAP_CACHE_H