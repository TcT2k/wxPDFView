#include "PDFViewBookmarksCtrl.h"

#include <wx/artprov.h>

#include "fpdfdoc/fpdf_doc.h"
#include "private/PDFViewImpl.h"

#include <vector>

class wxPDFViewBookmark: public std::vector<wxPDFViewBookmark>
{
public:
	wxPDFViewBookmark(CPDF_BookmarkTree& bmTree, CPDF_Bookmark& bookmark):
		m_bookmark(bookmark)
	{
		CFX_ByteString bookmarkTitleSDK = bookmark.GetTitle().UTF8Encode();
		m_title = wxString::FromUTF8(bookmarkTitleSDK, bookmarkTitleSDK.GetLength());
		CPDF_Bookmark child = bmTree.GetFirstChild(bookmark);
		while (child)
		{
			push_back(wxPDFViewBookmark(bmTree, child));
			child = bmTree.GetNextSibling(child);
		}
	}

	void Navigate(wxPDFView* pdfView)
	{
		CPDF_Document* doc = (CPDF_Document*) pdfView->GetImpl()->GetDocument();
		CPDF_Dest dest = m_bookmark.GetDest(doc);
		if (!dest)
		{
			CPDF_Action action = m_bookmark.GetAction();
			dest = action.GetDest(doc);
		}

		if (dest)
			pdfView->SetCurrentPage(dest.GetPageIndex(doc));
	}

	wxString m_title;
	CPDF_Bookmark m_bookmark;
};

class wxPDFViewBookmarksModel: public wxDataViewModel
{
public:
	wxPDFViewBookmarksModel(CPDF_Document* doc):
		m_tree(doc)
	{
		CPDF_Bookmark emptyBM;
		CPDF_Bookmark rootBM = m_tree.GetFirstChild(emptyBM);
		if (rootBM)
			m_rootBookmark = new wxPDFViewBookmark(m_tree, emptyBM);
		else
			m_rootBookmark = NULL;

		m_isEmpty = (m_rootBookmark == NULL) || m_rootBookmark->empty();
	}

	~wxPDFViewBookmarksModel()
	{
		delete m_rootBookmark;
	}

	virtual unsigned int GetColumnCount() const
	{
		return 1;
	}

	virtual wxString GetColumnType( unsigned int col ) const
	{
		return wxT("wxDataViewIconText");
	}

	virtual void GetValue( wxVariant &variant, const wxDataViewItem &item, unsigned int col ) const
	{
		wxPDFViewBookmark* bm = (wxPDFViewBookmark*) item.GetID();
		if (col == 0)
		{
			wxDataViewIconText data(bm->m_title, wxArtProvider::GetIcon(
				(bm->empty()) ? wxART_HELP_PAGE : wxART_HELP_FOLDER, wxART_OTHER));

			variant << data;
		}
	}

	virtual bool SetValue( const wxVariant &variant, const wxDataViewItem &item, unsigned int col )
	{
		return false;
	}

	virtual bool IsEnabled( const wxDataViewItem &item, unsigned int col ) const
	{
		return true;
	}

	virtual wxDataViewItem GetParent( const wxDataViewItem &item ) const
	{
		// the invisible root node has no parent
		if (!item.IsOk())
			return wxDataViewItem(0);

		return wxDataViewItem( (void*) NULL );
	}

	virtual bool IsContainer( const wxDataViewItem &item ) const
	{
		// the invisble root node can have children
		if (!m_rootBookmark)
			return false;
		if (!item.IsOk())
			return true;

		wxPDFViewBookmark* bm = (wxPDFViewBookmark*) item.GetID();
		return !bm->empty();
	}

	virtual unsigned int GetChildren( const wxDataViewItem &parent, wxDataViewItemArray &array ) const
	{
		wxPDFViewBookmark* bm = (wxPDFViewBookmark*) parent.GetID();
		if (!bm && m_rootBookmark)
			bm = m_rootBookmark;

		for (auto it = bm->begin(); it != bm->end(); ++it)
		{
			wxPDFViewBookmark* pChildBM = (wxPDFViewBookmark*) &(*it);
			array.Add( wxDataViewItem( (void*) pChildBM ) );
		}

		return bm->size();
	}

	void PrepareCtrl(wxDataViewCtrl* dataView)
	{
		if (m_rootBookmark)
		{
			for (auto it = m_rootBookmark->begin(); it != m_rootBookmark->end(); ++it)
			{
				wxPDFViewBookmark* pChildBM = (wxPDFViewBookmark*) &(*it);
				dataView->Expand( wxDataViewItem( (void*) pChildBM) );
			}
		}
	}

	bool m_isEmpty;

private:
	CPDF_BookmarkTree m_tree;
	wxPDFViewBookmark* m_rootBookmark;
};

//
// wxPDFViewBookmarksCtrl
//

bool wxPDFViewBookmarksCtrl::Create(wxWindow *parent,
					wxWindowID id,
					const wxPoint& pos,
					const wxSize& size,
					long style,
					const wxValidator& validator)
{
	if ( !wxDataViewCtrl::Create( parent, id, pos, size, style | wxDV_NO_HEADER, validator ) )
		return false;

	m_pdfView = NULL;
	m_isEmpty = true;
	Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &wxPDFViewBookmarksCtrl::OnSelectionChanged, this);
	Bind(wxEVT_SIZE, &wxPDFViewBookmarksCtrl::OnSize, this);

	AppendIconTextColumn
	(
		wxString(),					// no label (header is not shown anyhow)
		0,							// the only model column
		wxDATAVIEW_CELL_ACTIVATABLE,
		-1,							// default width
		wxALIGN_NOT,				//  and alignment
		0							// not resizable
	);

	return true;
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

void wxPDFViewBookmarksCtrl::OnSelectionChanged(wxDataViewEvent& event)
{
	wxPDFViewBookmark* bm = (wxPDFViewBookmark*) GetSelection().GetID();
	if (bm)
		bm->Navigate(m_pdfView);
}

void wxPDFViewBookmarksCtrl::OnPDFDocumentReady(wxCommandEvent& event)
{
	wxObjectDataPtr<wxPDFViewBookmarksModel> treeModel(new wxPDFViewBookmarksModel((CPDF_Document*) m_pdfView->GetImpl()->GetDocument()));
	AssociateModel(treeModel.get());
	treeModel->PrepareCtrl(this);
	m_isEmpty = treeModel->m_isEmpty;

	event.Skip();
}

void wxPDFViewBookmarksCtrl::OnSize(wxSizeEvent& event)
{
#if defined(wxUSE_GENERICDATAVIEWCTRL)
	// automatically resize our only column to take the entire control width
	if ( GetColumnCount() )
	{
		wxSize size = GetClientSize();
		GetColumn(0)->SetWidth(size.x);
	}
#endif
	event.Skip();
}
