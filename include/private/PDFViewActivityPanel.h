/////////////////////////////////////////////////////////////////////////////
// Name:        include/private/PDFViewActivityPanel.h
// Purpose:     wxPDFViewActivityPanel class
// Author:      Tobias Taschner
// Created:     2014-08-15
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_ACTIVITY_PANEL_H
#define PDFVIEW_ACTIVITY_PANEL_H

#include <wx/wx.h>
#include <wx/animate.h>

class wxPDFViewActivityPanel: public wxWindow
{
public:
	wxPDFViewActivityPanel() { }

	wxPDFViewActivityPanel(wxWindow *parent,
				wxWindowID winid = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = 0,
				const wxString& name = wxPanelNameStr)
	{
		Create(parent, winid, pos, size, style, name);
	}
	
	bool Create(wxWindow *parent,
				wxWindowID winid = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = 0,
				const wxString& name = wxPanelNameStr);
	
	void SetText(const wxString& text);
	
private:
	wxAnimationCtrl* m_aniCtrl;
	wxStaticText* m_staticText;
};


#endif // PDFVIEW_ACTIVITY_PANEL_H
