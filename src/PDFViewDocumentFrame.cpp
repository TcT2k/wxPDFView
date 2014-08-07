#include "PDFViewDocumentFrame.h"
#include <wx/artprov.h>
#include <cmath>
#include "PDFViewArtProvider.h"

wxPDFViewDocumentFrame::wxPDFViewDocumentFrame(wxWindow* parent, 
	const wxPoint &pos, 
	const wxSize &size, 
	long style, 
	const wxString &name):
	wxFrame(parent, wxID_ANY, _("PDF Viewer"), pos, size, style, name)
{
	wxPDFViewArtProvider::Initialize();

	this->SetSizeHints( wxSize( 800,600 ), wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

	wxBoxSizer* mainSizer = new wxBoxSizer( wxHORIZONTAL );
	mainSizer->SetMinSize( wxSize( 800,600 ) ); 

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE );
	m_splitter->SetSashPosition(180);
	m_splitter->SetMinimumPaneSize( 160 );

	m_navPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* navPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_docNotebook = new wxNotebook( m_navPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_docNotebook->SetMinSize( wxSize( 180,-1 ) );

	m_bookmarkPanel = new wxPanel( m_docNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bookmarkPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_pdfViewBookmarksCtrl = new wxPDFViewBookmarksCtrl(m_bookmarkPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	bookmarkPanelSizer->Add( m_pdfViewBookmarksCtrl, 1, wxALL|wxEXPAND, 2 );

	m_bookmarkPanel->SetSizer( bookmarkPanelSizer );
	m_bookmarkPanel->Layout();
	bookmarkPanelSizer->Fit( m_bookmarkPanel );
	m_docNotebook->AddPage( m_bookmarkPanel, _("Bookmarks"), true );
	m_thumbPanel = new wxPanel( m_docNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* thumbPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_thumbnailListBox = new wxPDFViewThumbnailListBox(m_thumbPanel, wxID_ANY);
	thumbPanelSizer->Add( m_thumbnailListBox, 1, wxALL|wxEXPAND, 5 );

	m_thumbPanel->SetSizer( thumbPanelSizer );
	m_thumbPanel->Layout();
	thumbPanelSizer->Fit( m_thumbPanel );
	m_docNotebook->AddPage( m_thumbPanel, _("Thumbnails"), false );

	navPanelSizer->Add( m_docNotebook, 1, wxEXPAND | wxALL, 3 );

	m_navPanel->SetSizer( navPanelSizer );
	m_navPanel->Layout();
	navPanelSizer->Fit( m_navPanel );
	m_docPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* docPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_pdfView = new wxPDFView(m_docPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	docPanelSizer->Add( m_pdfView, 1, wxEXPAND, 5 );

	m_docPanel->SetSizer( docPanelSizer );
	m_docPanel->Layout();
	docPanelSizer->Fit( m_docPanel );
	m_splitter->SplitVertically( m_navPanel, m_docPanel, 180 );
	mainSizer->Add( m_splitter, 1, wxEXPAND, 5 );

	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	m_toolBar = this->CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL, wxID_ANY );

	// Init search ctrl
	m_searchCtrl = new wxSearchCtrl(m_toolBar, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	m_searchCtrl->SetMinSize(wxSize(80, -1));
	m_searchCtrl->Bind(wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, &wxPDFViewDocumentFrame::OnSearchCtrlCancel, this);
	m_searchCtrl->Bind(wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, &wxPDFViewDocumentFrame::OnSearchCtrlFind, this);
	m_searchCtrl->Bind(wxEVT_COMMAND_TEXT_ENTER, &wxPDFViewDocumentFrame::OnSearchCtrlFind, this);
	m_searchCtrl->Bind(wxEVT_COMMAND_TEXT_UPDATED, &wxPDFViewDocumentFrame::OnSearchCtrlText, this);
	m_searchCtrl->Bind(wxEVT_KEY_UP, &wxPDFViewDocumentFrame::OnSearchCtrlKeyUp, this);
	m_searchTimer.Bind(wxEVT_TIMER, &wxPDFViewDocumentFrame::OnSearchTimerNotify, this);

	m_toolBar->AddTool(ID_NAVIGATION, _("Navigation"), wxArtProvider::GetBitmap(wxART_HELP_SIDE_PANEL, wxART_TOOLBAR), _("Show/Hide Navigation"), wxITEM_CHECK);
	m_toolBar->AddTool(wxID_PRINT, _("Print"), wxArtProvider::GetBitmap(wxART_PRINT, wxART_TOOLBAR), _("Print Document"));
	m_toolBar->AddSeparator();

	m_toolBar->AddTool(wxID_BACKWARD, _("Previous Page"), wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR), _("Show previous page"));
	m_toolBar->AddTool(wxID_FORWARD, _("Next Page"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR), _("Show next page"));
	m_pageTxtCtrl = new wxTextCtrl(m_toolBar, wxID_ANY, "0", wxDefaultPosition, wxSize(30, -1), wxTE_PROCESS_ENTER | wxTE_CENTRE);
	m_toolBar->AddControl(m_pageTxtCtrl);
	wxStaticText* pageCountDivTxt = new wxStaticText(m_toolBar, wxID_ANY, "/", wxDefaultPosition, wxSize(20, -1), wxALIGN_CENTRE_HORIZONTAL);
	m_toolBar->AddControl(pageCountDivTxt);
	m_pageCountTxtCtrl = new wxStaticText(m_toolBar, wxID_ANY, "0", wxDefaultPosition, wxSize(34, -1), wxALIGN_CENTRE_HORIZONTAL);
	m_toolBar->AddControl(m_pageCountTxtCtrl);

	m_zoomComboBox = new wxComboBox(m_toolBar, wxID_ANY, "100%", wxDefaultPosition, wxSize(80, -1), 0, NULL, wxTE_PROCESS_ENTER);
	m_zoomComboBox->Append("50%", (void*)50);
	m_zoomComboBox->Append("100%", (void*)100);
	m_zoomComboBox->Append("150%", (void*)150);
	m_zoomComboBox->Append("200%", (void*)200);
	m_zoomComboBox->Bind(wxEVT_COMBOBOX, &wxPDFViewDocumentFrame::OnZoomComboBox, this);

	m_toolBar->AddSeparator();
	m_toolBar->AddTool(ID_ZOOM_OUT, _("Zoom Out"), wxArtProvider::GetBitmap(wxART_PDFVIEW_ZOOM_OUT, wxART_TOOLBAR), _("Zoom Out"));
	m_toolBar->AddTool(ID_ZOOM_IN, _("Zoom In"), wxArtProvider::GetBitmap(wxART_PDFVIEW_ZOOM_IN, wxART_TOOLBAR), _("Zoom In"));
	m_toolBar->AddControl(m_zoomComboBox);

	m_toolBar->AddSeparator();
	m_toolBar->AddTool(ID_ZOOM_PAGE_FIT, _("Page Fit"), wxArtProvider::GetBitmap(wxART_PDFVIEW_PAGE_FIT, wxART_TOOLBAR), _("Fit one full page to window"), wxITEM_CHECK);
	m_toolBar->AddTool(ID_ZOOM_PAGE_WIDTH, _("Fit Width"), wxArtProvider::GetBitmap(wxART_PDFVIEW_PAGE_WIDTH, wxART_TOOLBAR), _("Fit to window width"), wxITEM_CHECK);

	m_toolBar->AddStretchableSpace();
	m_toolBar->AddControl(m_searchCtrl);

	m_toolBar->Realize();

	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentFrame::OnPageNextClick, this, wxID_FORWARD);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentFrame::OnPagePrevClick, this, wxID_BACKWARD);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentFrame::OnZoomInClick, this, ID_ZOOM_IN);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentFrame::OnZoomOutClick, this, ID_ZOOM_OUT);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentFrame::OnZoomPageFitClick, this, ID_ZOOM_PAGE_FIT);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentFrame::OnZoomPageWidthClick, this, ID_ZOOM_PAGE_WIDTH);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentFrame::OnNavigationClick, this, ID_NAVIGATION);

	Layout();

	this->Centre( wxBOTH );

	// Prepare for display while loading
	m_toolBar->EnableTool(wxID_FORWARD, false);
	m_toolBar->EnableTool(wxID_BACKWARD, false);
	m_pageTxtCtrl->Disable();
	m_toolBar->EnableTool(wxID_PRINT, false);
	m_toolBar->EnableTool(ID_ZOOM_OUT, false);
	m_toolBar->EnableTool(ID_ZOOM_IN, false);
	m_zoomComboBox->Disable();
	m_searchCtrl->Disable();

	m_toolBar->ToggleTool(ID_NAVIGATION, true);

	m_pdfView->Bind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewDocumentFrame::OnPDFDocumentReady, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_PAGE_CHANGED, &wxPDFViewDocumentFrame::OnPDFPageChanged, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_ZOOM_CHANGED, &wxPDFViewDocumentFrame::OnPDFZoomChanged, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_ZOOM_TYPE_CHANGED, &wxPDFViewDocumentFrame::OnPDFZoomTypeChanged, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_URL_CLICKED, &wxPDFViewDocumentFrame::OnPDFURLClicked, this);

	m_pdfViewBookmarksCtrl->SetPDFView(m_pdfView);
	m_thumbnailListBox->SetPDFView(m_pdfView);
}

wxPDFViewDocumentFrame::~wxPDFViewDocumentFrame()
{

}

bool wxPDFViewDocumentFrame::LoadFile(const wxString& fileName)
{
	return m_pdfView->LoadFile(fileName);
}

void wxPDFViewDocumentFrame::OnPDFPageChanged(wxCommandEvent& event)
{
	m_pageTxtCtrl->SetLabelText(wxString::Format("%d", event.GetInt() + 1));
	m_toolBar->EnableTool(wxID_BACKWARD, event.GetInt() > 0);
	m_toolBar->EnableTool(wxID_FORWARD, event.GetInt() < m_pdfView->GetPageCount() - 1);
}

void wxPDFViewDocumentFrame::OnPDFZoomChanged(wxCommandEvent& event)
{
	m_zoomComboBox->SetLabel(wxString::Format("%.1f%%", m_pdfView->GetZoom() * 100));
	m_toolBar->EnableTool(ID_ZOOM_IN, m_pdfView->GetZoom() < m_pdfView->GetMaxZoom());
	m_toolBar->EnableTool(ID_ZOOM_OUT, m_pdfView->GetZoom() > m_pdfView->GetMinZoom());
}

void wxPDFViewDocumentFrame::OnPDFZoomTypeChanged(wxCommandEvent& event)
{
	m_toolBar->ToggleTool(ID_ZOOM_PAGE_FIT, m_pdfView->GetZoomType() == wxPDFVIEW_ZOOM_TYPE_FIT_PAGE);
	m_toolBar->ToggleTool(ID_ZOOM_PAGE_WIDTH, m_pdfView->GetZoomType() == wxPDFVIEW_ZOOM_TYPE_PAGE_WIDTH);
}

void wxPDFViewDocumentFrame::OnPDFDocumentReady(wxCommandEvent& event)
{
	m_toolBar->EnableTool(ID_ZOOM_OUT, true);
	m_toolBar->EnableTool(ID_ZOOM_IN, true);
	m_zoomComboBox->Enable();

	m_pageCountTxtCtrl->SetLabelText(wxString::Format("%d", m_pdfView->GetPageCount()));
	m_pageTxtCtrl->Enable();
	m_searchCtrl->Enable();

	if (!m_pdfView->GetRootBookmark())
	{
		m_docNotebook->RemovePage(0);
	}
	m_pdfView->SetFocus();

	event.Skip();
}

void wxPDFViewDocumentFrame::OnPDFURLClicked(wxCommandEvent& event)
{
	if (!event.GetString().empty())
		wxLaunchDefaultBrowser(event.GetString());
}

void wxPDFViewDocumentFrame::OnNavigationClick(wxCommandEvent& event)
{
	if (m_toolBar->GetToolState(ID_NAVIGATION))
		m_splitter->SplitVertically(m_navPanel, m_docPanel, 180);
	else
		m_splitter->Unsplit(m_navPanel);
}

void wxPDFViewDocumentFrame::OnZoomInClick( wxCommandEvent& event )
{
	m_pdfView->SetZoom(m_pdfView->GetZoom() + 0.1);
}

void wxPDFViewDocumentFrame::OnZoomOutClick( wxCommandEvent& event )
{
	m_pdfView->SetZoom(m_pdfView->GetZoom() - 0.1);
}

void wxPDFViewDocumentFrame::OnPageNextClick( wxCommandEvent& event )
{
	m_pdfView->NavigateToPage(wxPDFVIEW_PAGE_NAV_NEXT);
}

void wxPDFViewDocumentFrame::OnPagePrevClick( wxCommandEvent& event )
{
	m_pdfView->NavigateToPage(wxPDFVIEW_PAGE_NAV_PREV);
}

void wxPDFViewDocumentFrame::OnZoomComboBox( wxCommandEvent& event )
{
	if (m_zoomComboBox->GetSelection() >= 0)
	{
		double zoom = reinterpret_cast<int>(m_zoomComboBox->GetClientData(m_zoomComboBox->GetSelection())) / (double) 100;
		m_pdfView->SetZoom(zoom);
	}
}

void wxPDFViewDocumentFrame::OnZoomPageFitClick( wxCommandEvent& event)
{
	m_pdfView->SetZoomType(wxPDFVIEW_ZOOM_TYPE_FIT_PAGE);
}

void wxPDFViewDocumentFrame::OnZoomPageWidthClick( wxCommandEvent& event)
{
	m_pdfView->SetZoomType(wxPDFVIEW_ZOOM_TYPE_PAGE_WIDTH);
}

void wxPDFViewDocumentFrame::OnSearchCtrlFind(wxCommandEvent& event)
{

}

void wxPDFViewDocumentFrame::OnSearchCtrlCancel(wxCommandEvent& event)
{

}

void wxPDFViewDocumentFrame::OnSearchCtrlText(wxCommandEvent& event)
{

}

void wxPDFViewDocumentFrame::OnSearchCtrlKeyUp(wxKeyEvent& event)
{

}

void wxPDFViewDocumentFrame::OnSearchTimerNotify( wxTimerEvent& event)
{

}
