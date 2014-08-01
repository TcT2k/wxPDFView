#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/sharedptr.h>
#include <wx/webview.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/filedlg.h>

#include "Simple.h"

bool SimpleApp::OnInit()
{
	SimpleFrame *frame = new SimpleFrame();
	frame->Show(true);

	return true;
}

SimpleFrame::SimpleFrame() : wxFrame(NULL, wxID_ANY, "wxPDFView")
{
	wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

	m_pdfView = new wxPDFView(this, wxID_ANY);

	wxButton* btn = new wxButton(this, wxID_ANY, "Load document");

	topsizer->Add(m_pdfView, 1, wxEXPAND);
	topsizer->Add(btn);

	this->SetSizer(topsizer);
	this->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SimpleFrame::OnButton, this);
}

void SimpleFrame::OnButton(wxCommandEvent &evt)
{
	wxFileDialog dlg(this, "Select Document", wxEmptyString, wxEmptyString, "*.pdf", wxCENTRE|wxFD_OPEN|wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK)
	{
		m_pdfView->LoadFile(dlg.GetPath());
	}
}
