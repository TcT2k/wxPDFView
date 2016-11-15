/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewDocumentPanel.cpp
// Purpose:     wxPDFViewDocumentPanel implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "PDFViewDocumentPanel.h"
#include "PDFViewArtProvider.h"
#include "private/PDFViewActivityPanel.h"

#include <wx/config.h>
#include <wx/filename.h>

wxString wxPDFViewDocumentPanel::DefaultName = "wxPDFView Document Panel";

wxPDFViewDocumentPanel::wxPDFViewDocumentPanel(wxWindow* parent, 
	wxWindowID id,
	const wxPoint &pos, 
	const wxSize &size, 
	long style, 
	const wxString &name)
{
	Create(parent, id, pos, size, style, name);
}

bool wxPDFViewDocumentPanel::Create(wxWindow* parent, 
	wxWindowID id,
	const wxPoint &pos, 
	const wxSize &size,
	long style,
	const wxString &name)
{
	if (!wxPanel::Create(parent, id, pos, size, style, name))
		return false;

	m_toolBar = NULL;
	
	// Set default config group
	wxPDFViewArtProvider::Initialize();

	SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

	wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
	mainSizer->SetMinSize( wxDLG_UNIT(this, wxSize( 400,300 )) );
	
	m_infoBar = new wxInfoBar(this);
	mainSizer->Add(m_infoBar, 0, wxEXPAND, 0 );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE );
	m_splitter->SetSashPosition(wxDLG_UNIT(this, wxSize(160, -1)).GetWidth());
    m_splitter->SetMinimumPaneSize(wxDLG_UNIT(this, wxSize(160, -1)).GetWidth());

	m_navPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* navPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_docNotebook = new wxNotebook( m_navPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
    m_docNotebook->SetMinSize(wxDLG_UNIT(this, wxSize(80, -1)));

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
	m_splitter->SplitVertically( m_navPanel, m_docPanel, 220 );
	mainSizer->Add( m_splitter, 1, wxEXPAND, 5 );

	SetSizerAndFit( mainSizer );

	Layout();

	// Bind PDF events
	m_pdfView->Bind(wxEVT_PDFVIEW_DOCUMENT_READY, &wxPDFViewDocumentPanel::OnPDFDocumentReady, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_PAGE_CHANGED, &wxPDFViewDocumentPanel::OnPDFPageChanged, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_ZOOM_CHANGED, &wxPDFViewDocumentPanel::OnPDFZoomChanged, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_ZOOM_TYPE_CHANGED, &wxPDFViewDocumentPanel::OnPDFZoomTypeChanged, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_URL_CLICKED, &wxPDFViewDocumentPanel::OnPDFURLClicked, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_REMOTE_GOTO, &wxPDFViewDocumentPanel::OnPDFRemoteGoto, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_LAUNCH, &wxPDFViewDocumentPanel::OnPDFLaunch, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_UNSUPPORTED_FEATURE, &wxPDFViewDocumentPanel::OnPDFUnsupportedFeature, this);
	m_pdfView->Bind(wxEVT_PDFVIEW_ACTIVITY, &wxPDFViewDocumentPanel::OnPDFActivity, this);

	m_pdfViewBookmarksCtrl->SetPDFView(m_pdfView);
	m_thumbnailListBox->SetPDFView(m_pdfView);

	UpdateSearchControls();
	
	ShowNavigationPane(false);
	
	// Init activity panel
	m_activityActive = false;
	m_activityPanel = new wxPDFViewActivityPanel(this);
	wxSizeEvent sizeEvent;
	OnFrameSize(sizeEvent);
	Bind(wxEVT_SIZE, &wxPDFViewDocumentPanel::OnFrameSize, this);
	m_activityPanel->Hide();

	SetName(DefaultName);

	return true;
}

wxPDFViewDocumentPanel::~wxPDFViewDocumentPanel()
{

}

void wxPDFViewDocumentPanel::SetToolBar(wxToolBar* toolBar)
{
	if (m_toolBar)
	{
		m_searchCtrl = NULL;
		m_pageTxtCtrl = NULL;
		m_pageCountTxtCtrl = NULL;
		m_zoomComboBox = NULL;

		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnPageNextClick, this, wxID_FORWARD);
		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnPagePrevClick, this, wxID_BACKWARD);
		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnZoomInClick, this, ID_ZOOM_IN);
		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnZoomOutClick, this, ID_ZOOM_OUT);
		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnZoomTypeClick, this, ID_ZOOM_PAGE_FIT, ID_ZOOM_PAGE_WIDTH);
		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnNavigationClick, this, ID_NAVIGATION);
		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnPrintClicked, this, wxID_PRINT);
		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnSearchNext, this, ID_FIND_NEXT);
		m_toolBar->Unbind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnSearchPrev, this, ID_FIND_PREV);
	}

	m_toolBar = toolBar;

	if (!m_toolBar)
		return;

	if (m_toolBar->GetToolsCount())
		m_toolBar->AddSeparator();

	// Init search ctrl
	m_searchCtrl = new wxSearchCtrl(m_toolBar, wxID_ANY, wxString(), wxDefaultPosition, wxDLG_UNIT(this, wxSize(100, -1)), wxTE_PROCESS_ENTER);
	m_searchCtrl->SetMinSize(wxDLG_UNIT(this, wxSize(100, -1)));
	m_searchCtrl->Bind(wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, &wxPDFViewDocumentPanel::OnSearchCtrlCancel, this);
	m_searchCtrl->Bind(wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, &wxPDFViewDocumentPanel::OnSearchCtrlFind, this);
	m_searchCtrl->Bind(wxEVT_COMMAND_TEXT_ENTER, &wxPDFViewDocumentPanel::OnSearchCtrlFind, this);
	m_searchCtrl->Bind(wxEVT_COMMAND_TEXT_UPDATED, &wxPDFViewDocumentPanel::OnSearchCtrlText, this);

	m_toolBar->AddTool(ID_NAVIGATION, _("Navigation"), GetToolbarBitmap(wxART_HELP_SIDE_PANEL), _("Show/Hide Navigation"), wxITEM_CHECK);
	m_toolBar->AddTool(wxID_PRINT, _("Print"), GetToolbarBitmap(wxART_PRINT), _("Print Document"));
	m_toolBar->AddSeparator();

	m_toolBar->AddTool(wxID_BACKWARD, _("Previous Page"), GetToolbarBitmap(wxART_GO_BACK), _("Show previous page"));
	m_toolBar->AddTool(wxID_FORWARD, _("Next Page"), GetToolbarBitmap(wxART_GO_FORWARD), _("Show next page"));
	m_pageTxtCtrl = new wxTextCtrl(m_toolBar, wxID_ANY, "0", wxDefaultPosition, wxDLG_UNIT(this, wxSize(24, -1)), wxTE_PROCESS_ENTER | wxTE_RIGHT);
	m_pageTxtCtrl->Bind(wxEVT_TEXT_ENTER, &wxPDFViewDocumentPanel::OnPageTextEnter, this);
	m_toolBar->AddControl(m_pageTxtCtrl);
	wxStaticText* pageCountDivTxt = new wxStaticText(m_toolBar, wxID_ANY, "/", wxDefaultPosition, wxDLG_UNIT(this, wxSize(8, -1)), wxALIGN_CENTRE_HORIZONTAL);
	m_toolBar->AddControl(pageCountDivTxt);
	m_pageCountTxtCtrl = new wxStaticText(m_toolBar, wxID_ANY, "0", wxDefaultPosition, wxDLG_UNIT(this, wxSize(24, -1)), wxALIGN_LEFT);
	m_toolBar->AddControl(m_pageCountTxtCtrl);

	m_zoomComboBox = new wxComboBox(m_toolBar, wxID_ANY, "100%", wxDefaultPosition, wxDLG_UNIT(this, wxSize(52, -1)), 0, NULL, wxTE_PROCESS_ENTER);
	m_zoomComboBox->Append("50%", (void*)50);
	m_zoomComboBox->Append("100%", (void*)100);
	m_zoomComboBox->Append("150%", (void*)150);
	m_zoomComboBox->Append("200%", (void*)200);
	m_zoomComboBox->Bind(wxEVT_COMBOBOX, &wxPDFViewDocumentPanel::OnZoomComboBox, this);
	m_zoomComboBox->Bind(wxEVT_TEXT_ENTER, &wxPDFViewDocumentPanel::OnZoomTextEnter, this);

	m_toolBar->AddSeparator();
	m_toolBar->AddTool(ID_ZOOM_OUT, _("Zoom Out"), GetToolbarBitmap(wxART_PDFVIEW_ZOOM_OUT), _("Zoom Out"));
	m_toolBar->AddTool(ID_ZOOM_IN, _("Zoom In"), GetToolbarBitmap(wxART_PDFVIEW_ZOOM_IN), _("Zoom In"));
	m_toolBar->AddControl(m_zoomComboBox);

	m_toolBar->AddSeparator();
	m_toolBar->AddTool(ID_ZOOM_PAGE_FIT, _("Page Fit"), GetToolbarBitmap(wxART_PDFVIEW_PAGE_FIT), _("Fit one full page to window"), wxITEM_CHECK);
	m_toolBar->AddTool(ID_ZOOM_PAGE_WIDTH, _("Fit Width"), GetToolbarBitmap(wxART_PDFVIEW_PAGE_WIDTH), _("Fit to window width"), wxITEM_CHECK);
	m_toolBar->AddSeparator();

	m_toolBar->AddTool(ID_DISPLAY_SINGLE_PAGE, _("Single Page"), GetToolbarBitmap(wxART_PDFVIEW_SINGLE_PAGE), _("Show a single page"), wxITEM_RADIO);
	m_toolBar->AddTool(ID_DISPLAY_2PAGES, _("Two Pages"), GetToolbarBitmap(wxART_PDFVIEW_TWO_PAGES), _("Show two pages side by side"), wxITEM_RADIO);
	m_toolBar->AddTool(ID_DISPLAY_2PAGES_COVER, _("Two Pages (Cover)"), GetToolbarBitmap(wxART_PDFVIEW_TWO_PAGES_COVER), _("Show two pages side by side with cover"), wxITEM_RADIO);

	m_toolBar->AddStretchableSpace();
	m_toolBar->AddControl(m_searchCtrl);
	m_toolBar->AddTool(ID_FIND_NEXT, _("Next"), GetToolbarBitmap(wxART_GO_DOWN), _("Find next"));
	m_toolBar->AddTool(ID_FIND_PREV, _("Previous"), GetToolbarBitmap(wxART_GO_UP), _("Find previous"));

	m_toolBar->Realize();

	// Bind toolbar events
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnPageNextClick, this, wxID_FORWARD);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnPagePrevClick, this, wxID_BACKWARD);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnZoomInClick, this, ID_ZOOM_IN);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnZoomOutClick, this, ID_ZOOM_OUT);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnZoomTypeClick, this, ID_ZOOM_PAGE_FIT, ID_ZOOM_PAGE_WIDTH);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnDisplayTypeClick, this, ID_DISPLAY_SINGLE_PAGE, ID_DISPLAY_2PAGES_COVER);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnNavigationClick, this, ID_NAVIGATION);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnPrintClicked, this, wxID_PRINT);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnSearchNext, this, ID_FIND_NEXT);
	m_toolBar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &wxPDFViewDocumentPanel::OnSearchPrev, this, ID_FIND_PREV);

	// Prepare for display while loading
	m_toolBar->EnableTool(wxID_FORWARD, false);
	m_toolBar->EnableTool(wxID_BACKWARD, false);
	m_pageTxtCtrl->Disable();
	m_toolBar->EnableTool(wxID_PRINT, false);
	m_toolBar->EnableTool(ID_ZOOM_OUT, false);
	m_toolBar->EnableTool(ID_ZOOM_IN, false);
	m_zoomComboBox->Disable();
	m_searchCtrl->Disable();
	m_toolBar->EnableTool(ID_NAVIGATION, false);

	UpdateSearchControls();
}

void wxPDFViewDocumentPanel::OnFrameSize(wxSizeEvent& event)
{
	wxSize clientSize = GetClientSize();
	wxSize panelSize = m_activityPanel->GetSize();
	
	int margin = 32;
	
	m_activityPanel->SetPosition(wxPoint(clientSize.x - panelSize.x - margin, clientSize.y - panelSize.y - margin));
	
	event.Skip();
}

wxBitmap wxPDFViewDocumentPanel::GetToolbarBitmap(wxArtID id)
{
	return wxArtProvider::GetBitmap(id, wxART_TOOLBAR);
}

bool wxPDFViewDocumentPanel::LoadFile(const wxString& fileName)
{
	m_fileName = fileName;
	return m_pdfView->LoadFile(fileName);
}

void wxPDFViewDocumentPanel::OnPDFPageChanged(wxCommandEvent& event)
{
	m_pageTxtCtrl->SetLabelText(wxString::Format("%d", event.GetInt() + 1));
	if (m_toolBar)
	{
		m_toolBar->EnableTool(wxID_BACKWARD, event.GetInt() > 0);
		m_toolBar->EnableTool(wxID_FORWARD, event.GetInt() < m_pdfView->GetPageCount() - 1);
	}
}

void wxPDFViewDocumentPanel::OnPDFZoomChanged(wxCommandEvent& event)
{
	m_zoomComboBox->SetLabel(wxString::Format("%.1f%%", m_pdfView->GetZoom() * 100));
	if (m_toolBar)
	{
		m_toolBar->EnableTool(ID_ZOOM_IN, m_pdfView->GetZoom() < m_pdfView->GetMaxZoom());
		m_toolBar->EnableTool(ID_ZOOM_OUT, m_pdfView->GetZoom() > m_pdfView->GetMinZoom());
	}

	event.Skip();
}

void wxPDFViewDocumentPanel::OnPDFZoomTypeChanged(wxCommandEvent& event)
{
	if (m_toolBar)
	{
		for (int toolId = ID_ZOOM_PAGE_FIT; toolId <= ID_ZOOM_PAGE_WIDTH; toolId++)
			m_toolBar->ToggleTool(toolId, m_pdfView->GetZoomType() == toolId - ID_ZOOM_PAGE_FIT + 1);
	}

	event.Skip();
}

void wxPDFViewDocumentPanel::OnPDFDocumentReady(wxCommandEvent& event)
{
	wxConfigBase* cfg = wxConfig::Get();
	wxConfigPathChanger pathChanger(cfg, GetName() + "/");
	
	if (m_toolBar)
	{
		m_toolBar->EnableTool(ID_ZOOM_OUT, true);
		m_toolBar->EnableTool(ID_ZOOM_IN, true);
		m_toolBar->EnableTool(ID_NAVIGATION, true);
		m_zoomComboBox->Enable();
		m_toolBar->EnableTool(wxID_PRINT, m_pdfView->IsPrintAllowed());

		m_pageCountTxtCtrl->SetLabelText(wxString::Format("%d", m_pdfView->GetPageCount()));
		m_pageTxtCtrl->Enable();
		m_searchCtrl->Enable();
	}

	int bmPage = m_docNotebook->FindPage(m_bookmarkPanel);
	if (!m_pdfView->GetRootBookmark())
	{
		if (bmPage != wxNOT_FOUND)
			m_docNotebook->RemovePage(bmPage);
	} 
	else if (bmPage == wxNOT_FOUND) 
	{
		m_docNotebook->InsertPage(0, m_bookmarkPanel, _("Bookmarks"));
	}
	
	bool showNavPane = cfg->ReadBool("ShowNavigation", m_pdfView->GetRootBookmark() != NULL);
	ShowNavigationPane(showNavPane);
	
	m_pdfView->SetFocus();
	
	m_pdfView->SetZoomType((wxPDFViewZoomType) cfg->ReadLong("ZoomType2", wxPDFVIEW_ZOOM_TYPE_FIT_PAGE));
	m_pdfView->SetDisplayFlags(cfg->ReadLong("DisplayFlags", 0));
	if (m_pdfView->GetDisplayFlags() & wxPDFVIEW_DISPLAY_TWO_PAGE_COVER)
		m_toolBar->ToggleTool(ID_DISPLAY_2PAGES_COVER, true);
	else if (m_pdfView->GetDisplayFlags() & wxPDFVIEW_DISPLAY_TWO_PAGE)
		m_toolBar->ToggleTool(ID_DISPLAY_2PAGES, true);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnPDFURLClicked(wxCommandEvent& event)
{
	if (!event.GetString().empty())
		wxLaunchDefaultBrowser(event.GetString());

	event.Skip();
}

void wxPDFViewDocumentPanel::OnPDFRemoteGoto(wxCommandEvent& event)
{
	wxFileName targetFileName(m_fileName);
	targetFileName.SetFullName(event.GetString());
	// TODO: handle page number
	LoadFile(targetFileName.GetFullPath());
}

void wxPDFViewDocumentPanel::OnPDFLaunch(wxCommandEvent& event)
{
	wxFileName targetFileName(m_fileName);
	targetFileName.SetFullName(event.GetString());
	wxLaunchDefaultApplication(targetFileName.GetFullPath());
}

void wxPDFViewDocumentPanel::OnPDFUnsupportedFeature(wxCommandEvent& event)
{
	m_infoBar->ShowMessage(wxString::Format(_("Unsupported PDF feature: %s"), event.GetString()), wxICON_INFORMATION);
}

void wxPDFViewDocumentPanel::StartActivity(const wxString& description)
{
	m_activityActive = true;
	wxBeginBusyCursor();
	UpdateActivity(description);
}

void wxPDFViewDocumentPanel::UpdateActivity(const wxString& description)
{
	m_activityPanel->SetText(description);
	wxSizeEvent sizeEvt;
	OnFrameSize(sizeEvt);
	m_activityPanel->Show();
}

void wxPDFViewDocumentPanel::EndActivity()
{
	m_activityActive = false;
	m_activityPanel->Hide();
	if (wxIsBusy())
		wxEndBusyCursor();
}

void wxPDFViewDocumentPanel::OnPDFActivity(wxCommandEvent& event)
{
	wxString desc = event.GetString();
	if (!desc.empty())
		StartActivity(desc);
	else
		EndActivity();
}

void wxPDFViewDocumentPanel::ShowNavigationPane(bool show)
{
	if (show)
        m_splitter->SplitVertically(m_navPanel, m_docPanel, wxDLG_UNIT(this, wxSize(160, -1)).x);
	else
		m_splitter->Unsplit(m_navPanel);
	if (m_toolBar)
		m_toolBar->ToggleTool(ID_NAVIGATION, show);
}

void wxPDFViewDocumentPanel::OnNavigationClick(wxCommandEvent& event)
{
	bool showNavigation = m_toolBar->GetToolState(ID_NAVIGATION);
	ShowNavigationPane(showNavigation);
	
	wxConfigBase* cfg = wxConfig::Get();
	wxConfigPathChanger pathChanger(cfg, GetName() + "/");
	cfg->Write("ShowNavigation", showNavigation);
	
	event.Skip();
}

void wxPDFViewDocumentPanel::OnPrintClicked(wxCommandEvent& event)
{
	CallAfter(&wxPDFViewDocumentPanel::StartPrint);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnZoomInClick( wxCommandEvent& event )
{
	m_pdfView->SetZoom(m_pdfView->GetZoom() + 0.1);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnZoomOutClick( wxCommandEvent& event )
{
	m_pdfView->SetZoom(m_pdfView->GetZoom() - 0.1);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnPageNextClick( wxCommandEvent& event )
{
	m_pdfView->NavigateToPage(wxPDFVIEW_PAGE_NAV_NEXT);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnPagePrevClick( wxCommandEvent& event )
{
	m_pdfView->NavigateToPage(wxPDFVIEW_PAGE_NAV_PREV);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnPageTextEnter( wxCommandEvent& event )
{
	int newPage = wxAtoi(m_pageTxtCtrl->GetValue());
	if (newPage > 0)
	{
		m_pdfView->GoToPage(newPage - 1);
		m_pdfView->SetFocus();
	}

	event.Skip();
}

void wxPDFViewDocumentPanel::OnZoomComboBox( wxCommandEvent& event )
{
	if (m_zoomComboBox->GetSelection() >= 0)
	{
		wxString zoomStr = m_zoomComboBox->GetValue();

		double zoom = wxAtof(zoomStr) / 100.0;
		m_pdfView->SetZoom(zoom);
		m_pdfView->SetFocus();
	}
	SaveZoomConfig();

	event.Skip();
}

void wxPDFViewDocumentPanel::OnZoomTextEnter( wxCommandEvent& event )
{
	wxString zoomStr = m_zoomComboBox->GetValue();

	double newZoom = wxAtof(zoomStr);
	if (newZoom > 0)
	{
		m_pdfView->SetZoom(newZoom / 100);
		m_pdfView->SetFocus();
	}
	SaveZoomConfig();

	event.Skip();
}

void wxPDFViewDocumentPanel::OnZoomTypeClick( wxCommandEvent& event)
{
	m_pdfView->SetZoomType(static_cast<wxPDFViewZoomType>(event.GetId() - ID_ZOOM_PAGE_FIT + 1));
	SaveZoomConfig();

	event.Skip();
}

void wxPDFViewDocumentPanel::OnDisplayTypeClick( wxCommandEvent& event)
{

	switch (event.GetId())
	{
		case ID_DISPLAY_SINGLE_PAGE:
			m_pdfView->SetDisplayFlags(0);
			break;
		case ID_DISPLAY_2PAGES:
			m_pdfView->SetDisplayFlags(wxPDFVIEW_DISPLAY_TWO_PAGE);
			break;
		case ID_DISPLAY_2PAGES_COVER:
			m_pdfView->SetDisplayFlags(wxPDFVIEW_DISPLAY_TWO_PAGE | wxPDFVIEW_DISPLAY_TWO_PAGE_COVER);
			break;
		default:
			break;
	}

	wxConfigBase* cfg = wxConfig::Get();
	wxConfigPathChanger pathChanger(cfg, GetName() + "/");
	cfg->Write("DisplayFlags", (int) m_pdfView->GetDisplayFlags());
}

void wxPDFViewDocumentPanel::OnSearchCtrlFind(wxCommandEvent& event)
{
	Find(m_searchCtrl->GetValue(), true);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnSearchCtrlText(wxCommandEvent& event)
{
	m_searchText = m_searchCtrl->GetValue();

	UpdateSearchControls();

	event.Skip();
}

void wxPDFViewDocumentPanel::OnSearchCtrlCancel(wxCommandEvent& event)
{
	Find("", true);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnSearchNext(wxCommandEvent& event)
{
	Find(m_searchText, true);

	event.Skip();
}

void wxPDFViewDocumentPanel::OnSearchPrev(wxCommandEvent& event)
{
	Find(m_searchText, false);

	event.Skip();
}

void wxPDFViewDocumentPanel::Find(const wxString& text, bool forward)
{
	m_searchText = text;
	UpdateSearchControls();

	int flags = wxPDFVIEW_FIND_DEFAULT;
	if (!forward)
		flags = flags | wxPDFVIEW_FIND_BACKWARDS;
	if (m_pdfView->Find(m_searchText, flags) == wxNOT_FOUND && !m_searchText.empty())
		wxMessageBox(_("Text not found"), _("Warning"), wxICON_WARNING | wxOK, this);
}

void wxPDFViewDocumentPanel::UpdateSearchControls()
{
	if (m_toolBar)
	{
		m_toolBar->EnableTool(ID_FIND_NEXT, !m_searchText.empty());
		m_toolBar->EnableTool(ID_FIND_PREV, !m_searchText.empty());
	}
}

void wxPDFViewDocumentPanel::StartPrint()
{
	wxPrintDialogData printDialogData = m_pdfView->GetPrintDialogData();
	PreparePrintDialogData(printDialogData);
	wxPrinter printer(&printDialogData);
	wxSharedPtr<wxPrintout> printout(m_pdfView->CreatePrintout());
	if (!printer.Print(this, printout.get()))
	{
		if (printer.GetLastError() == wxPRINTER_ERROR)
			wxLogError(_("Document could not be printed"));
	}
}

void wxPDFViewDocumentPanel::SaveZoomConfig()
{
	wxConfigBase* cfg = wxConfig::Get();
	wxConfigPathChanger pathChanger(cfg, GetName() + "/");
	cfg->Write("ZoomType2", (int) m_pdfView->GetZoomType());
}
