/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewBookmarksCtrl.cpp
// Purpose:     wxPDFViewBookmarksCtrl implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "PDFViewBookmarksCtrl.h"

#include <wx/artprov.h>

class wxPDFViewBookmarksModel: public wxDataViewModel
{
public:
	wxPDFViewBookmarksModel(const wxPDFViewBookmark* root):
		m_rootBookmark(root)
	{
	}

	virtual unsigned int GetColumnCount() const
	{
		return 1;
	}

	virtual wxString GetColumnType( unsigned int WXUNUSED(col) ) const
	{
		return wxT("wxDataViewIconText");
	}

	virtual void GetValue( wxVariant &variant, const wxDataViewItem &item, unsigned int col ) const
	{
		wxPDFViewBookmark* bm = (wxPDFViewBookmark*) item.GetID();
		if (col == 0)
		{
			wxDataViewIconText data(bm->GetTitle(), wxArtProvider::GetIcon(
				(bm->empty()) ? wxART_HELP_PAGE : wxART_HELP_FOLDER, wxART_OTHER));

			variant << data;
		}
	}

	virtual bool SetValue( const wxVariant &WXUNUSED(variant), const wxDataViewItem &WXUNUSED(item), unsigned int WXUNUSED(col) )
	{
		return false;
	}

	virtual bool IsEnabled( const wxDataViewItem &WXUNUSED(item), unsigned int WXUNUSED(col) ) const
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
		const wxPDFViewBookmark* bm = (const wxPDFViewBookmark*) parent.GetID();
		if (!bm && m_rootBookmark)
			bm = m_rootBookmark;
		if (!bm)
			return 0;

		for (auto it = bm->begin(); it != bm->end(); ++it)
		{
			wxPDFViewBookmark* pChildBM = it->get();
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
				wxPDFViewBookmark* pChildBM = it->get();
				dataView->Expand( wxDataViewItem( (void*) pChildBM) );
			}
		}
	}

private:
	const wxPDFViewBookmark* m_rootBookmark;
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
		m_pdfView->Unbind(wxEVT_PDFVIEW_DOCUMENT_CLOSED, &wxPDFViewBookmarksCtrl::OnPDFDocumentClosed, this);
	}

	m_pdfView = pdfView;

	if (m_pdfView != NULL)
	{
		// Connect events
		m_pdfView->Bind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewBookmarksCtrl::OnPDFDocumentReady, this);
		m_pdfView->Bind(wxEVT_PDFVIEW_DOCUMENT_CLOSED, &wxPDFViewBookmarksCtrl::OnPDFDocumentClosed, this);
	}
}

void wxPDFViewBookmarksCtrl::OnSelectionChanged(wxDataViewEvent& event)
{
	wxPDFViewBookmark* bm = (wxPDFViewBookmark*) GetSelection().GetID();
	if (bm)
		bm->Navigate(m_pdfView);

	event.Skip();
}

void wxPDFViewBookmarksCtrl::OnPDFDocumentClosed(wxCommandEvent& event)
{
#ifdef __WXOSX__
	AssociateModel(NULL);
#else
	// Workaround for potential bug in generic dataview impl
	wxObjectDataPtr<wxPDFViewBookmarksModel> treeModel(new wxPDFViewBookmarksModel(NULL));
	AssociateModel(treeModel.get());
#endif

	event.Skip();
}

void wxPDFViewBookmarksCtrl::OnPDFDocumentReady(wxCommandEvent& event)
{
	wxObjectDataPtr<wxPDFViewBookmarksModel> treeModel(new wxPDFViewBookmarksModel(m_pdfView->GetRootBookmark()));
	AssociateModel(treeModel.get());
	treeModel->PrepareCtrl(this);

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
