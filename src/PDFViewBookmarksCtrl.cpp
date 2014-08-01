#include "PDFViewBookmarksCtrl.h"

#include "fpdfdoc/fpdf_doc.h"

wxClientData* CreateBookmarkClientData(CPDF_Bookmark& bookmark, CPDF_Document* doc)
{
	CPDF_Dest dest = bookmark.GetDest(doc);
	if (!dest)
	{
		CPDF_Action action = bookmark.GetAction();
		dest = action.GetDest(doc);
	}

	if (dest)
		return new wxStringClientData(wxString::Format("%d", dest.GetPageIndex(doc)));
	else
		return NULL;
}

int AddBookmarkToViewItem(CPDF_BookmarkTree& bmTree, wxPDFViewBookmarksCtrl& ctrl, const wxDataViewItem& viewItem, CPDF_Bookmark& bookmark)
{
	int bookmarksAdded = 1;
	CPDF_Bookmark firstChild = bmTree.GetFirstChild(bookmark);
	wxClientData* clientData = CreateBookmarkClientData(bookmark, (CPDF_Document*) ctrl.GetPDFView()->GetDocument());
	if (firstChild)
	{
		wxDataViewItem containerItem = ctrl.AppendContainer(viewItem, (const wchar_t*) bookmark.GetTitle(), -1, -1, clientData);

		// Enumerate child nodes
		CPDF_Bookmark bm = firstChild;
		while (bm)
		{
			bookmarksAdded += AddBookmarkToViewItem(bmTree, ctrl, containerItem, bm);
			bm = bmTree.GetNextSibling(bm);
		}
		ctrl.Expand(containerItem);
	} else
		ctrl.AppendItem(viewItem, (const wchar_t*) bookmark.GetTitle(), -1, clientData);

	return bookmarksAdded;
}

//
// wxPDFViewBookmarksCtrl
//

wxPDFViewBookmarksCtrl::wxPDFViewBookmarksCtrl(wxWindow *parent,
					wxWindowID id,
					const wxPoint& pos,
					const wxSize& size,
					long style,
					const wxValidator& validator):
	wxDataViewTreeCtrl(parent, id, pos, size, style, validator)
{
	Init();
}

void wxPDFViewBookmarksCtrl::SetPDFView(wxPDFView* pdfView)
{
	if (m_pdfView != NULL)
	{
		// Disconnect events
		m_pdfView->Unbind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewBookmarksCtrl::OnPDFDocumentReady, this);
	}

	m_pdfView = pdfView;

	if (m_pdfView != NULL)
	{
		// Connect events
		m_pdfView->Bind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewBookmarksCtrl::OnPDFDocumentReady, this);
	}
}

void wxPDFViewBookmarksCtrl::Init()
{
	m_pdfView = NULL;
	m_isEmpty = true;
	Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &wxPDFViewBookmarksCtrl::OnSelectionChanged, this);
	Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &wxPDFViewBookmarksCtrl::OnStartEditing, this);
}

void wxPDFViewBookmarksCtrl::OnSelectionChanged(wxDataViewEvent& event)
{
	wxDataViewItem selectedItem = GetSelection();
	wxClientData* clientData = GetItemData(selectedItem);
	if (clientData)
	{
		wxString dest = static_cast<wxStringClientData*>(clientData)->GetData();
		if (m_pdfView)
			m_pdfView->GotoPage(wxAtoi(dest));
	}
}

void wxPDFViewBookmarksCtrl::OnStartEditing(wxDataViewEvent& event)
{
	event.Veto();
}

void wxPDFViewBookmarksCtrl::UpdateDocumentBookmarks()
{
	DeleteAllItems();

	CPDF_BookmarkTree bmTree((CPDF_Document*)m_pdfView->GetDocument());
	CPDF_Bookmark emptyBM;
	CPDF_Bookmark rootBM = bmTree.GetFirstChild(emptyBM);
	if (rootBM)
	{
		wxDataViewItem rootViewItem;
		int bookmarksAdded = AddBookmarkToViewItem(bmTree, *this, rootViewItem, rootBM);
		m_isEmpty = bookmarksAdded < 2;
	} else
		m_isEmpty = true;
}

void wxPDFViewBookmarksCtrl::OnPDFDocumentReady(wxCommandEvent& event)
{
	UpdateDocumentBookmarks();

	event.Skip();
}
