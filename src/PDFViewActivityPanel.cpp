/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewActivityPanel.cpp
// Purpose:     wxPDFViewActivityPanel class
// Author:      Tobias Taschner
// Created:     2014-08-15
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "private/PDFViewActivityPanel.h"
#include "../art/activity.h"

#include <wx/mstream.h>

bool wxPDFViewActivityPanel::Create(wxWindow *parent,
			wxWindowID winid,
			const wxPoint& pos,
			const wxSize& size,
			long style,
			const wxString& name)
{
	if (!wxWindow::Create(parent, winid, pos, size, style, name))
		return false;
	
	wxMemoryInputStream mIStr(activity_gif, sizeof(activity_gif));
	wxAnimation ani;
	ani.Load(mIStr);
	wxASSERT_MSG(ani.IsOk(), "Throbber animation invalid");
	SetBackgroundColour(wxColour(110, 110, 110));
	
	// Create sub controls
	wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	
	m_aniCtrl = new wxAnimationCtrl(this, wxID_ANY, ani);
	m_aniCtrl->Play();
	sizer->Add(m_aniCtrl, wxSizerFlags().Border(wxALL, 2).Center());
	
	m_staticText = new wxStaticText(this, wxID_ANY, _("Processing..."));
	m_staticText->SetForegroundColour(*wxLIGHT_GREY);
	sizer->Add(m_staticText, wxSizerFlags().Border(wxALL, 2).Center());
	
	SetSizerAndFit(sizer);
	
	return true;
}

void wxPDFViewActivityPanel::SetText(const wxString& text)
{
	m_staticText->SetLabelText(text);
	Fit();
}
