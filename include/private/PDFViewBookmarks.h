#ifndef PDFVIEW_BOOKMARKS_H
#define PDFVIEW_BOOKMARKS_H

#include "PDFView.h"
#include "fpdfdoc.h"
#include "fpdfdoc/fpdf_doc.h"

class wxPDFViewBookmarks
{
public:
	wxPDFViewBookmarks(FPDF_DOCUMENT doc);

	const wxPDFViewBookmark* GetRoot() const { return m_root.get(); };

private:
	CPDF_BookmarkTree m_tree;
	wxSharedPtr<wxPDFViewBookmark> m_root;
};

#endif // PDFVIEW_BOOKMARKS_H
