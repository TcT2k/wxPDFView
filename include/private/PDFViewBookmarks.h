/////////////////////////////////////////////////////////////////////////////
// Name:        include/private/PDFView.h
// Purpose:     wxPDFViewBookmarks class
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_BOOKMARKS_H
#define PDFVIEW_BOOKMARKS_H

#include "PDFView.h"
#include "fpdf_doc.h"

class wxPDFViewBookmarks
{
public:
	wxPDFViewBookmarks(FPDF_DOCUMENT doc);

	const wxPDFViewBookmark* GetRoot() const { return m_root.get(); };

private:
	wxSharedPtr<wxPDFViewBookmark> m_root;
};

#endif // PDFVIEW_BOOKMARKS_H
