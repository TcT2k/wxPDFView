#ifndef PDFVIEW_H
#define PDFVIEW_H

#include <wx/wx.h>
#include <wx/sharedptr.h>
#include <wx/vector.h>
#include <istream>

class wxPDFViewImpl;

enum wxPDFViewZoomType
{
	wxPDFVIEW_ZOOM_TYPE_FREE,
	wxPDFVIEW_ZOOM_TYPE_FIT_PAGE,
	wxPDFVIEW_ZOOM_TYPE_PAGE_WIDTH
};

enum wxPDFViewPageNavigation
{
	wxPDFVIEW_PAGE_NAV_NEXT,
	wxPDFVIEW_PAGE_NAV_PREV,
	wxPDFVIEW_PAGE_NAV_FIRST,
	wxPDFVIEW_PAGE_NAV_LAST
};

enum wxPDFViewFindFlags
{
	wxPDFVIEW_FIND_MATCH_CASE = 0x0001,
	wxPDFVIEW_FIND_BACKWARDS = 0x0002,
	wxPDFVIEW_FIND_DEFAULT = 0
};

class wxPDFViewBookmark;

class wxPDFView: public wxScrolledCanvas
{
public:
	wxPDFView() { };

	wxPDFView(wxWindow *parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxScrolledWindowStyle,
		const wxString& name = wxPanelNameStr);

	virtual ~wxPDFView();

	bool Create(wxWindow *parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxScrolledWindowStyle,
		const wxString& name = wxPanelNameStr);

	void NavigateToPage(wxPDFViewPageNavigation pageNavigation);

	int GetPageCount() const;

	const wxPDFViewBookmark* GetRootBookmark() const;

	void SetCurrentPage(int pageIndex);

	int GetCurrentPage() const;

	void SetZoom(double zoom);

	double GetZoom() const;

	double GetMaxZoom() const;

	double GetMinZoom() const;

	void SetZoomType(wxPDFViewZoomType zoomType);

	wxPDFViewZoomType GetZoomType() const;

	// Scrolls next result into view and returns the number of matches or wxNOT_FOUND
	long Find(const wxString& text, int flags = wxPDFVIEW_FIND_DEFAULT);

	bool LoadFile(const wxString& fileName);

	bool LoadStream(wxSharedPtr<std::istream> pStream);

	void CloseDocument();

	wxPDFViewImpl* GetImpl() const { return m_impl; };

	virtual void ScrollWindow(int dx, int dy, const wxRect *rect = NULL);

private:
	wxPDFViewImpl* m_impl;

	friend class wxPDFViewImpl;
};

class wxPDFViewBookmark: public wxVector< wxSharedPtr<wxPDFViewBookmark> >
{
public:
	virtual wxString GetTitle() const = 0;
	virtual void Navigate(wxPDFView* pdfView) = 0;
};

wxDECLARE_EVENT(wxEVT_PDFVIEW_DOCUMENT_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_PAGE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_ZOOM_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_ZOOM_TYPE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_URL_CLICKED, wxCommandEvent);

#endif // PDFVIEW_H
