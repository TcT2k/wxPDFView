#ifndef PDFVIEW_H
#define PDFVIEW_H

#include <wx/wx.h>
#include <wx/sharedptr.h>
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

	void SetCurrentPage(int pageIndex);

	int GetCurrentPage() const;

	void SetZoom(double zoom);

	double GetZoom() const;

	double GetMaxZoom() const;

	double GetMinZoom() const;

	void SetZoomType(wxPDFViewZoomType zoomType);

	wxPDFViewZoomType GetZoomType() const;

	bool LoadFile(const wxString& fileName);

	bool LoadStream(wxSharedPtr<std::istream> pStream);

	void CloseDocument();

	wxPDFViewImpl* GetImpl() const { return m_impl; };

	virtual void ScrollWindow(int dx, int dy, const wxRect *rect = NULL);

private:
	wxPDFViewImpl* m_impl;

	friend class wxPDFViewImpl;
};

wxDECLARE_EVENT(wxEVT_PDFVIEW_DOCUMENT_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_PAGE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_ZOOM_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_ZOOM_TYPE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PDFVIEW_URL_CLICKED, wxCommandEvent);

#endif // PDFVIEW_H
