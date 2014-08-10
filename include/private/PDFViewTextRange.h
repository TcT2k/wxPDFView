/////////////////////////////////////////////////////////////////////////////
// Name:        include/private/PDFViewTextRange.h
// Purpose:     wxPDFViewTextRange class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_TEXTRANGE_H
#define PDFVIEW_TEXTRANGE_H

#include <wx/vector.h>
#include <wx/gdicmn.h>

class wxPDFViewPage;

class wxPDFViewTextRange
{
public:
	wxPDFViewTextRange(wxPDFViewPage* page, int charIndex, int charCount);

	wxPDFViewPage* GetPage() const { return m_page; }
	int GetCharIndex() const { return m_charIndex; }
	int GetCharCount() const { return m_charCount; }
	void SetCharCount(int count) { m_charCount = count; }

	wxVector<wxRect> GetScreenRects(const wxRect& pageRect);

private:
	wxPDFViewPage* m_page;
	int m_charIndex;
	int m_charCount;

	wxVector<wxRect> m_cachedScreenRects;
};

#endif // PDFVIEW_TEXTRANGE_H