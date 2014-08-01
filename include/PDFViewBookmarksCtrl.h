#ifndef PDFVIEW_BOOKMARKS_CTRL_H
#define PDFVIEW_BOOKMARKS_CTRL_H

#include <wx/dataview.h>

#include "PDFView.h"

class wxPDFViewBookmarksCtrl: public wxDataViewTreeCtrl
{
public:
	wxPDFViewBookmarksCtrl(wxWindow *parent,
					   wxWindowID id,
					   const wxPoint& pos = wxDefaultPosition,
					   const wxSize& size = wxDefaultSize,
					   long style = wxDV_NO_HEADER | wxDV_ROW_LINES,
					   const wxValidator& validator = wxDefaultValidator);

	void SetPDFView(wxPDFView* pdfView);

	wxPDFView* GetPDFView() const { return m_pdfView; };

	bool IsEmpty() const { return m_isEmpty; };

protected:

private:
	wxPDFView* m_pdfView;
	bool m_isEmpty;

	void Init();

	void UpdateDocumentBookmarks();

	void OnPDFDocumentReady(wxCommandEvent& event);

	void OnSelectionChanged(wxDataViewEvent& event);

	void OnStartEditing(wxDataViewEvent& event);
};

#endif // PDFVIEW_BOOKMARKS_CTRL_H