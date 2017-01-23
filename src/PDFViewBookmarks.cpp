/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewBookmarks.cpp
// Purpose:     wxPDFViewBookmarks implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "private/PDFViewBookmarks.h"
#include "private/PDFViewImpl.h"

class wxPDFViewBookmarkImpl: public wxPDFViewBookmark
{
public:
	wxPDFViewBookmarkImpl(FPDF_DOCUMENT doc, FPDF_BOOKMARK bookmark):
		m_bookmark(bookmark)
	{
		unsigned long length = FPDFBookmark_GetTitle(bookmark, NULL, 0);
		if (length > 0)
		{
			char * buffer = new char[length];
			length = FPDFBookmark_GetTitle(bookmark, buffer, length);
			m_title = wxString(buffer, wxCSConv(wxFONTENCODING_UTF16LE), length);
			delete [] buffer;
		}

		FPDF_BOOKMARK child = FPDFBookmark_GetFirstChild(doc, bookmark);
		while (child)
		{
			wxSharedPtr<wxPDFViewBookmark> newBM(new wxPDFViewBookmarkImpl(doc, child));
			push_back(newBM);
			child = FPDFBookmark_GetNextSibling(doc, child);
		}
	}

	virtual wxString GetTitle() const
	{
		return m_title;
	}

	virtual void Navigate(wxPDFView* pdfView)
	{
		FPDF_DOCUMENT doc = pdfView->GetImpl()->GetDocument();
		FPDF_ACTION action = FPDFBookmark_GetAction(m_bookmark);
		FPDF_DEST dest = NULL;
		if (action)
		{
			if (FPDFAction_GetType(action) == PDFACTION_GOTO)
			{
				dest = FPDFBookmark_GetDest(doc, m_bookmark);
			}
		} else {
			dest = FPDFBookmark_GetDest(doc, m_bookmark);
		}
		if (dest)
		{
			unsigned long pageIndex = FPDFDest_GetPageIndex(doc, dest);
			pdfView->GoToPage(pageIndex);
		}
	}

private:
	wxString m_title;
	FPDF_BOOKMARK m_bookmark;
};

wxPDFViewBookmarks::wxPDFViewBookmarks(FPDF_DOCUMENT doc)
{
	FPDF_BOOKMARK emptyBM = NULL;
	FPDF_BOOKMARK rootBM = FPDFBookmark_GetFirstChild(doc, emptyBM);
	if (rootBM)
		m_root.reset(new wxPDFViewBookmarkImpl(doc, emptyBM));
}

