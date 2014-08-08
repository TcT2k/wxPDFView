#include "private/PDFViewTextRange.h"

#include "private/PDFViewPages.h"

#include "fpdftext.h"

wxPDFViewTextRange::wxPDFViewTextRange(wxPDFViewPage* page, int charIndex, int charCount)
{
	m_page = page;
	m_charIndex = charIndex;
	m_charCount = charCount;
}

wxVector<wxRect> wxPDFViewTextRange::GetScreenRects(const wxRect& pageRect)
{
	if (!m_cachedScreenRects.empty())
		return m_cachedScreenRects;

	int charIndex = m_charIndex;
	int charCount = m_charCount;
	if (charCount < 0) 
	{
		charCount *= -1;
		charIndex -= charCount - 1;
	}

	int count = FPDFText_CountRects(m_page->GetTextPage(), charIndex, charCount);
	for (int i = 0; i < count; ++i) 
	{
		double left, top, right, bottom;
		FPDFText_GetRect(m_page->GetTextPage(), i, &left, &top, &right, &bottom);
		m_cachedScreenRects.push_back(
			m_page->PageToScreen(pageRect, left, top, right, bottom));
	}

  return m_cachedScreenRects;
}

