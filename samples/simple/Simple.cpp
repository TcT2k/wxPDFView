/////////////////////////////////////////////////////////////////////////////
// Name:        samples/simple/Simple.h
// Purpose:     Simple sample app implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/sharedptr.h>
#include <wx/webview.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/filedlg.h>

#include "Simple.h"

#include "PDFViewDocumentFrame.h"

bool SimpleApp::OnInit()
{
	SimpleFrame *frame = new SimpleFrame();
	frame->Show(true);

	return true;
}

SimpleFrame::SimpleFrame() : wxFrame(NULL, wxID_ANY, "wxPDFView")
{
	wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

	wxButton* btn = new wxButton(this, wxID_ANY, "Load document");

	topsizer->Add(btn, 0, wxCENTER);

	this->SetSizer(topsizer);
	this->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SimpleFrame::OnButton, this);
}

void SimpleFrame::OnButton(wxCommandEvent &evt)
{
	wxFileDialog dlg(this, "Select Document", wxEmptyString, wxEmptyString, "*.pdf", wxCENTRE|wxFD_OPEN|wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK)
	{
		wxPDFViewDocumentFrame* frame = new wxPDFViewDocumentFrame(this);

		if (frame->LoadFile(dlg.GetPath()))
			frame->Show();
	}
}
