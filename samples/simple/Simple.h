#ifndef _SIMPLE_H_
#define _SIMPLE_H_

#include <wx/app.h>
#include <wx/frame.h>

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
};

wxIMPLEMENT_APP(SimpleApp);

#endif
