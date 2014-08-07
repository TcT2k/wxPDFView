#ifndef PDFVIEW_BOOKMARKS_CTRL_H
#define PDFVIEW_BOOKMARKS_CTRL_H

#include <wx/dataview.h>

#include "PDFView.h"

class wxPDFViewBookmarksCtrl: public wxDataViewCtrl
{
public:
	wxPDFViewBookmarksCtrl() { };

	wxPDFViewBookmarksCtrl(wxWindow *parent,
							wxWindowID id,
							const wxPoint& pos = wxDefaultPosition,
							const wxSize& size = wxDefaultSize,
							long style = wxDV_ROW_LINES,
							const wxValidator& validator = wxDefaultValidator)
	{
		Create(parent, id, pos, size, style, validator);
	};

	bool Create(wxWindow *parent,
			wxWindowID id,
			const wxPoint& pos = wxDefaultPosition,
			const wxSize& size = wxDefaultSize,
			long style = wxDV_ROW_LINES,
			const wxValidator& validator = wxDefaultValidator);

	void SetPDFView(wxPDFView* pdfView);

	wxPDFView* GetPDFView() const { return m_pdfView; };

protected:

private:
	wxPDFView* m_pdfView;

	void OnPDFDocumentReady(wxCommandEvent& event);

	void OnSelectionChanged(wxDataViewEvent& event);

	void OnSize(wxSizeEvent& event);
};

#endif // PDFVIEW_BOOKMARKS_CTRL_H
