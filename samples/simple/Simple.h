#ifndef _SIMPLE_H_
#define _SIMPLE_H_

#include <wx/app.h>
#include <wx/frame.h>

#include "PDFView.h"

class SimpleApp : public wxApp
{
public:
	virtual bool OnInit();
};

class SimpleFrame : public wxFrame
{
public:
	SimpleFrame();
	void OnButton(wxCommandEvent &evt);

private:
	wxPDFView* m_pdfView;
};

wxIMPLEMENT_APP(SimpleApp);

#endif
