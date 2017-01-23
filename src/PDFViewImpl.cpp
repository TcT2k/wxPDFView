/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewImpl.cpp
// Purpose:     wxPDFViewImpl implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "private/PDFViewImpl.h"
#include "private/PDFViewPrintout.h"

#include <wx/dcbuffer.h>
#include <wx/filename.h>

#include "fpdf_ext.h"
#include "fpdf_text.h"

#include "v8.h"
#include "libplatform/libplatform.h"

// See Table 3.20 in
// http://www.adobe.com/devnet/acrobat/pdfs/pdf_reference_1-7.pdf
#define PDF_PERMISSION_PRINT_LOW_QUALITY	1 << 2
#define PDF_PERMISSION_PRINT_HIGH_QUALITY	1 << 11
#define PDF_PERMISSION_COPY					1 << 4
#define PDF_PERMISSION_COPY_ACCESSIBLE		1 << 9

std::map<FPDF_FORMFILLINFO*, wxPDFViewImpl*> g_pdfFormMap;

void LogPDFError(unsigned long error)
{
	wxString errorMsg;
	switch (error)
	{
	case FPDF_ERR_UNKNOWN:
		errorMsg = _("Unknown Error");
		break;
	case FPDF_ERR_FILE:
		errorMsg = _("File not found or could not be opened.");
		break;
	case FPDF_ERR_FORMAT:
		errorMsg = _("File not in PDF format or corrupted.");
		break;
	case FPDF_ERR_PASSWORD:
		errorMsg = _("Password required or incorrect password.");
		break;
	case FPDF_ERR_SECURITY:
		errorMsg = _("Unsupported security scheme.");
		break;
	case FPDF_ERR_PAGE:
		errorMsg = _("Page not found or content error.");
		break;
	default:
		errorMsg = wxString::Format(_("Unknown Error (%d)"), error);
		break;
	};
	if (error != FPDF_ERR_SUCCESS)
		wxLogError("PDF Error: %s", errorMsg);
}

int Form_Alert(IPDF_JSPLATFORM* WXUNUSED(pThis), FPDF_WIDESTRING Msg, FPDF_WIDESTRING Title, int Type, int Icon)
{
	long msgBoxStyle = wxCENTRE;
	switch (Icon)
	{
		case 0:
			msgBoxStyle |= wxICON_ERROR;
			break;
		case 1:
			msgBoxStyle |= wxICON_WARNING;
			break;
		case 2:
			msgBoxStyle |= wxICON_QUESTION;
			break;
		case 3:
			msgBoxStyle |= wxICON_INFORMATION;
			break;
		default:
			break;
	}

	switch (Type)
	{
		case 0:
			msgBoxStyle |= wxOK;
			break;
		case 1:
			msgBoxStyle |= wxOK | wxCANCEL;
			break;
		case 2:
			msgBoxStyle |= wxYES_NO;
			break;
		case 3:
			msgBoxStyle |= wxYES_NO | wxCANCEL;
			break;
	}

	wxMBConvUTF16 conv;
	wxString msgTitle = conv.cMB2WC((char*) Title);
	wxString msgMsg = conv.cMB2WC((char*) Msg);

	int msgBoxRes = wxMessageBox(msgMsg, msgTitle, msgBoxStyle);

	int retVal = 0;
	switch (msgBoxRes)
	{
		case wxOK:
			retVal = 1;
			break;
		case wxCANCEL:
			retVal = 2;
			break;
		case wxNO:
			retVal = 3;
			break;
		case wxYES:
			retVal = 4;
			break;
	};

	return retVal;
}

void Form_GotoPage(IPDF_JSPLATFORM* pThis, int pageNumber)
{
	wxPDFViewImpl* impl = g_pdfFormMap[(FPDF_FORMFILLINFO*) pThis->m_pFormfillinfo];
	if (!impl)
		return;

	impl->GoToPage(pageNumber);
}

void Form_Print(IPDF_JSPLATFORM* pThis,
				  FPDF_BOOL bUI,
				  int nStart,
				  int nEnd,
				  FPDF_BOOL bSilent,
				  FPDF_BOOL bShrinkToFit,
				  FPDF_BOOL bPrintAsImage,
				  FPDF_BOOL bReverse,
				  FPDF_BOOL bAnnotations)
{
	wxPDFViewImpl* impl = g_pdfFormMap[(FPDF_FORMFILLINFO*) pThis->m_pFormfillinfo];
	if (!impl)
		return;

	// TODO: use parameters
	impl->Print();
}

wxString g_formSelectedFilePath;

int Form_Browse(IPDF_JSPLATFORM* pThis,
					void* filePath,
					int length)
{
	if (length == 0)
	{
		wxFileDialog dlg(NULL, _("Open File"), "", "", "All Files|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (dlg.ShowModal() == wxID_OK)
		{
			g_formSelectedFilePath = dlg.GetPath();
			return g_formSelectedFilePath.length();
		}
	}
	else if (filePath != NULL)
	{
		memcpy(filePath, g_formSelectedFilePath.utf8_str(), g_formSelectedFilePath.length());
		return g_formSelectedFilePath.length();
	}

	return 0;
}

void Form_OpenDoc(IPDF_JSPLATFORM* pThis,
				   FPDF_WIDESTRING Path)
{
	wxPDFViewImpl* impl = g_pdfFormMap[(FPDF_FORMFILLINFO*) pThis->m_pFormfillinfo];
	if (!impl)
		return;

	wxMBConvUTF16 conv;
	wxString filePath = conv.cMB2WC((char*) Path);

	wxLogDebug("Form_OpenDoc: %s", filePath);

	impl->GoToRemote(filePath);
}


wxPDFViewImpl* g_unsupportedHandlerPDFViewImpl = NULL;

void Unsupported_Handler(UNSUPPORT_INFO*, int type)
{
	if (g_unsupportedHandlerPDFViewImpl)
		g_unsupportedHandlerPDFViewImpl->HandleUnsupportedFeature(type);
}

UNSUPPORT_INFO g_unsupported_info = {
	1,
	Unsupported_Handler
};

int Get_Block(void* param, unsigned long pos, unsigned char* pBuf,
			  unsigned long size) 
{
	wxPDFViewImpl* impl = (wxPDFViewImpl*) param;
	std::istream* pIstr = impl->GetStream();
	pIstr->seekg(pos);
	pIstr->read((char*) pBuf, size);
	if (pIstr->gcount() == size && !pIstr->fail())
		return 1;
	else
		return 0;
}

FPDF_BOOL Is_Data_Avail(_FX_FILEAVAIL* WXUNUSED(pThis), size_t WXUNUSED(offset), size_t WXUNUSED(size))
{
  return true;
}

void Add_Segment(FX_DOWNLOADHINTS* WXUNUSED(pThis), size_t WXUNUSED(offset), size_t WXUNUSED(size))
{
}

void FFI_SetCursor(FPDF_FORMFILLINFO* pThis, int nCursorType)
{
	wxPDFViewImpl* impl = g_pdfFormMap[pThis];
	if (!impl)
		return;

	wxStockCursor cursorType = wxCURSOR_ARROW;

	switch (nCursorType) {
		case FXCT_NESW:
			cursorType = wxCURSOR_SIZENESW;
			break;
		case FXCT_NWSE:
			cursorType = wxCURSOR_SIZENWSE;
			break;
		case FXCT_VBEAM:
		case FXCT_HBEAM:
			cursorType = wxCURSOR_CHAR;
			break;
		case FXCT_HAND:
			cursorType = wxCURSOR_HAND;
			break;
	}

	wxLogDebug("FFI_SetCursor: %d", nCursorType);
	impl->SetDefaultCursor(cursorType);
}

void FFI_Invalidate(FPDF_FORMFILLINFO* pThis,
					   FPDF_PAGE page,
					   double left,
					   double top,
					   double right,
					   double bottom)
{
	wxPDFViewImpl* impl = g_pdfFormMap[pThis];
	if (impl)
	{
		int mostVisible = impl->GetMostVisiblePage();
		if (mostVisible >= 0 && (*impl->GetPages())[mostVisible].GetPage() == page)
		{
			wxRect rect(left, top, 0, 0);
			rect.SetBottom(bottom);
			rect.SetRight(right);
			impl->InvalidatePage(mostVisible, &rect);
		}
	}
}

FPDF_PAGE FFI_GetPage(FPDF_FORMFILLINFO* pThis, FPDF_DOCUMENT document, int nPageIndex)
{
	wxPDFViewImpl* impl = g_pdfFormMap[pThis];
	if (impl)
		return (*impl->GetPages())[nPageIndex].GetPage();
	return NULL;
}

FPDF_PAGE FFI_GetCurrentPage(FPDF_FORMFILLINFO* pThis, FPDF_DOCUMENT document)
{
	wxPDFViewImpl* impl = g_pdfFormMap[pThis];
	if (impl)
	{
		int mostVisible = impl->GetMostVisiblePage();
		if (mostVisible >= 0)
			return (*impl->GetPages())[mostVisible].GetPage();
	}
	return NULL;
}

void FFI_DoGoToAction(FPDF_FORMFILLINFO* pThis,
						 int nPageIndex,
						 int zoomMode,
						 float* fPosArray,
						 int sizeofArray)
{
	wxPDFViewImpl* impl = g_pdfFormMap[pThis];
	if (impl)
		impl->DoGoToAction(nPageIndex);
}

void FFI_ExecuteNamedAction(FPDF_FORMFILLINFO* pThis,
							   FPDF_BYTESTRING namedAction)
{
	wxString action(namedAction);
	wxPDFViewImpl* impl = g_pdfFormMap[pThis];
	if (impl)
		impl->ExecuteNamedAction(action);
}

//
// wxPDFViewActivity
//
class wxPDFViewActivity
{
public:
	wxPDFViewActivity(wxPDFViewImpl* pdfViewImpl, const wxString& description):
		m_impl(pdfViewImpl)
	{
		m_impl->SendActivity(description);
	}
	
	~wxPDFViewActivity()
	{
		m_impl->SendActivity("");
	}
	
private:
	wxPDFViewImpl* m_impl;
};

//
// wxPDFViewImpl
//

wxAtomicInt wxPDFViewImpl::ms_pdfSDKRefCount = 0;
bool wxPDFViewImpl::ms_v8initialized = false;

wxPDFViewImpl::wxPDFViewImpl(wxPDFView* ctrl):
	m_ctrl(ctrl),
	m_handCursor(wxCURSOR_HAND),
	m_defaultCursor(wxCURSOR_ARROW)
{
	AcquireSDK();

	SetPages(&m_pages);

	m_printValidator = NULL;
	
	m_zoomType = wxPDFVIEW_ZOOM_TYPE_FREE;
	m_pagePadding = 16;
	m_scrollStepX = 20;
	m_scrollStepY = 20;
	m_zoom = 1.0;
	m_minZoom = 0.1;
	m_maxZoom = 10.0;
	m_pDataStream.reset();
	m_pdfDoc = NULL;
	m_pdfForm = NULL;
	m_pdfAvail = NULL;
	m_mostVisiblePage = -1;
	m_bookmarks = NULL;
	m_currentFindIndex = -1;
	m_docPermissions = 0;
	m_linearized = false;
	m_backPage = -1;

	// PDF SDK structures
	memset(&m_pdfFileAccess, '\0', sizeof(m_pdfFileAccess));
	m_pdfFileAccess.m_FileLen = 0;
	m_pdfFileAccess.m_GetBlock = Get_Block;
	m_pdfFileAccess.m_Param = this;

	memset(&m_pdfFileAvail, '\0', sizeof(m_pdfFileAvail));
	m_pdfFileAvail.version = 1;
	m_pdfFileAvail.IsDataAvail = Is_Data_Avail;
	
	memset(&m_hints, '\0', sizeof(m_hints));
	m_hints.version = 1;
	m_hints.AddSegment = Add_Segment;

	m_ctrl->Bind(wxEVT_PAINT, &wxPDFViewImpl::OnPaint, this);
	m_ctrl->Bind(wxEVT_SIZE, &wxPDFViewImpl::OnSize, this);
	m_ctrl->Bind(wxEVT_MOUSEWHEEL, &wxPDFViewImpl::OnMouseWheel, this);
	m_ctrl->Bind(wxEVT_MOTION, &wxPDFViewImpl::OnMouseMotion, this);
	m_ctrl->Bind(wxEVT_LEFT_UP, &wxPDFViewImpl::OnMouseLeftUp, this);
	m_ctrl->Bind(wxEVT_LEFT_DOWN, &wxPDFViewImpl::OnMouseLeftDown, this);
	m_ctrl->Bind(wxEVT_KEY_DOWN, &wxPDFViewImpl::OnKeyDown, this);
	m_ctrl->Bind(wxEVT_KEY_UP, &wxPDFViewImpl::OnKeyUp, this);
	m_ctrl->Bind(wxEVT_CHAR, &wxPDFViewImpl::OnKeyChar, this);
}

wxPDFViewImpl::~wxPDFViewImpl()
{
	CloseDocument();

	ReleaseSDK();
}

void wxPDFViewImpl::NavigateToPage(wxPDFViewPageNavigation pageNavigation)
{
	switch (pageNavigation)
	{
		case wxPDFVIEW_PAGE_NAV_NEXT:
		{
			int nextPage = GetMostVisiblePage();
			if (GetPagePosition(nextPage) == wxPDFVIEW_PAGE_POS_LEFT)
				nextPage++;
			GoToPage(nextPage + 1);
			break;
		}
		case wxPDFVIEW_PAGE_NAV_PREV:
			GoToPage(GetMostVisiblePage() - 1);
			break;
		case wxPDFVIEW_PAGE_NAV_FIRST:
			GoToPage(0);
			break;
		case wxPDFVIEW_PAGE_NAV_LAST:
			GoToPage(GetPageCount() - 1);
			break;
	}
}

void wxPDFViewImpl::UpdateDocumentInfo()
{
	UpdateVirtualSize();
	CalcZoomLevel();

	wxCommandEvent readyEvent(wxEVT_PDFVIEW_DOCUMENT_READY);
	m_ctrl->ProcessEvent(readyEvent);
	wxCommandEvent pgEvent(wxEVT_PDFVIEW_PAGE_CHANGED);
	pgEvent.SetInt(0);
	m_ctrl->ProcessEvent(pgEvent);
}

void wxPDFViewImpl::RecalculatePageRects()
{
	m_pageRects.clear();
	m_pageRects.reserve(GetPageCount());
	
	wxSize defaultPageSize = wxDefaultSize;
	m_maxPageHeight = 0;

#ifdef __WXMSW__
	HDC desktopDc = ::GetDC(NULL);
	int dpiX = ::GetDeviceCaps(desktopDc, LOGPIXELSX);
	double screenScale = dpiX / (double)96;
#endif

	m_docSize.Set(0, 0);
	wxRect pageRect;
	wxRect prevPageRect;
	for (int i = 0; i < GetPageCount(); ++i)
	{
		bool pageAvail = !m_linearized || FPDFAvail_IsPageAvail(m_pdfAvail, i, &m_hints) != 0;
		wxPDFViewPagePosition pagePos = GetPagePosition(i);

		wxSize pageSize;
		double width;
		double height;
		if (pageAvail && FPDF_GetPageSizeByIndex(m_pdfDoc, i, &width, &height))
		{
			pageSize = wxSize(width, height);
			if (!defaultPageSize.IsFullySpecified())
				defaultPageSize = pageSize;
		} else
			pageSize = defaultPageSize;
		
#ifdef __WXMSW__
		pageSize *= screenScale;
#endif

		if (pagePos != wxPDFVIEW_PAGE_POS_RIGHT)
			pageRect.y += m_pagePadding / 2;
		pageRect.SetSize(pageSize);
		m_pageRects.push_back(pageRect);
		
		int pageWidth = pageSize.x;
		if (pagePos != wxPDFVIEW_PAGE_POS_CENTER && i > 0)
			pageWidth *= 2;
		
		if (pageWidth > m_docSize.x)
			m_docSize.x = pageWidth;

		if (pageSize.y > m_maxPageHeight)
			m_maxPageHeight = pageSize.y;

		if (pagePos != wxPDFVIEW_PAGE_POS_LEFT)
		{
			pageRect.x = 0;
			if (prevPageRect.height > pageSize.y)
				pageRect.y += prevPageRect.height;
			else
				pageRect.y += pageSize.y;
			pageRect.y += m_pagePadding / 2;
			
			// Make sure every page top is pixel exact scrollable
			int pageDisplayHeight = pageRect.height + m_pagePadding;
			int scrollMod = pageDisplayHeight % m_scrollStepY;
			if (scrollMod)
				pageRect.y += m_scrollStepY - scrollMod;
		}
		else
			pageRect.x += pageSize.x;

		prevPageRect = pageRect;
	}
	m_docSize.SetHeight(pageRect.y - (m_pagePadding / 2));
	
	AlignPageRects();
}

void wxPDFViewImpl::AlignPageRects()
{
	int ctrlWidth = m_ctrl->GetVirtualSize().GetWidth() / m_ctrl->GetScaleX();
	
	int pageIndex = 0;
	for (auto it = m_pageRects.begin(); it != m_pageRects.end(); ++it, ++pageIndex)
	{
		switch (GetPagePosition(pageIndex))
		{
			case wxPDFVIEW_PAGE_POS_CENTER:
				it->x = (ctrlWidth - it->width) / 2;
				break;
			case wxPDFVIEW_PAGE_POS_LEFT:
				it->x = (ctrlWidth / 2) - it->width;
				break;
			case wxPDFVIEW_PAGE_POS_RIGHT:
				it->x = (ctrlWidth / 2);
				break;
		}
	}
}

void wxPDFViewImpl::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxAutoBufferedPaintDC dc(m_ctrl);
	m_ctrl->PrepareDC(dc);

	wxSharedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));

	wxRect rectUpdate = m_ctrl->GetUpdateClientRect();
	rectUpdate.SetPosition(m_ctrl->CalcUnscrolledPosition(rectUpdate.GetPosition()));
	rectUpdate = ScaledToUnscaled(rectUpdate);

	dc.SetBackground(m_ctrl->GetBackgroundColour());
	dc.Clear();

	// Draw visible pages
	if (GetFirstVisiblePage() < 0)
		return;

	for (int pageIndex = GetFirstVisiblePage(); pageIndex <= GetLastVisiblePage(); ++pageIndex)
	{
		wxRect pageRect = m_pageRects[pageIndex];
		if (pageRect.Intersects(rectUpdate))
		{
			m_pages[pageIndex].Draw(this, dc, *gc, pageRect);
			if (GetPagePosition(pageIndex) == wxPDFVIEW_PAGE_POS_RIGHT)
			{
				// Draw line between left and right page
				gc->SetPen(*wxLIGHT_GREY_PEN);
				wxPoint2DDouble linePoints[2] = {
					{ (wxDouble)pageRect.x, (wxDouble)pageRect.y },
					{ (wxDouble)pageRect.x, (wxDouble)pageRect.y + pageRect.height }
				};
				gc->DrawLines(2, linePoints);
			}
		}
	}

	// Draw text selections
	gc->SetBrush(wxColor(0, 0, 200, 50));
	gc->SetPen(*wxTRANSPARENT_PEN);

	for (auto it = m_selection.begin(); it != m_selection.end(); ++it)
	{
		int pageIndex = it->GetPage()->GetIndex();
		if (IsPageVisible(pageIndex))
		{
			wxRect pageRect = m_pageRects[pageIndex];
			if (pageRect.Intersects(rectUpdate))
			{
				// Screen rects are relative to the page
				wxVector<wxRect> screenRects = it->GetScreenRects(m_pageRects[pageIndex]);
				for (auto sr = screenRects.begin(); sr != screenRects.end(); ++sr)
				{
					sr->Offset(m_pageRects[pageIndex].GetPosition());
					gc->DrawRectangle(sr->x, sr->y, sr->width, sr->height);
				}
			}
		}
	}
}

void wxPDFViewImpl::OnSize(wxSizeEvent& event)
{
	AlignPageRects();
	CalcZoomLevel();
	CalcVisiblePages();
	event.Skip();
}

void wxPDFViewImpl::OnMouseWheel(wxMouseEvent& event)
{
	if (event.ControlDown() && event.GetWheelRotation() != 0)
	{
		double currentZoom = m_zoom;

		double delta;
		if ( currentZoom < 100 )
			delta = 0.05;
		else if ( currentZoom <= 120 )
			delta = 0.1;
		else
			delta = 0.5;

		if ( event.GetWheelRotation() < 0 )
			delta = -delta;

		SetZoom(currentZoom + delta);
	} else
		event.Skip();
}

void wxPDFViewImpl::OnMouseMotion(wxMouseEvent& event)
{
	if (EvaluateLinkTargetPageAtClientPos(event.GetPosition(), event.GetEventType()))
		m_ctrl->SetCursor(m_handCursor);
	else
		m_ctrl->SetCursor(m_defaultCursor);

	event.Skip();
}

void wxPDFViewImpl::OnMouseLeftDown(wxMouseEvent& event)
{
	EvaluateLinkTargetPageAtClientPos(event.GetPosition(), event.GetEventType());

	event.Skip();
}

void wxPDFViewImpl::OnMouseLeftUp(wxMouseEvent& event)
{
	if (EvaluateLinkTargetPageAtClientPos(event.GetPosition(), event.GetEventType()))
		m_ctrl->SetCursor(m_defaultCursor);

	event.Skip();
}

void wxPDFViewImpl::OnKeyUp(wxKeyEvent& event)
{
	FORM_OnKeyUp(m_pdfForm, m_pages[GetMostVisiblePage()].GetPage(), event.GetKeyCode(), 0);
	event.Skip();
}

void wxPDFViewImpl::OnKeyDown(wxKeyEvent& event)
{
	FORM_OnKeyDown(m_pdfForm, m_pages[GetMostVisiblePage()].GetPage(), event.GetKeyCode(), 0);
	event.Skip();
}

void wxPDFViewImpl::OnKeyChar(wxKeyEvent& event)
{
	FORM_OnChar(m_pdfForm, m_pages[GetMostVisiblePage()].GetPage(), event.GetKeyCode(), 0);
	event.Skip();
}

void wxPDFViewImpl::GoToPage(int pageIndex, const wxRect* centerRect)
{
	if (pageIndex < 0)
		pageIndex = 0;
	else if (pageIndex >= GetPageCount())
		pageIndex = GetPageCount() - 1;

	m_backPage = GetMostVisiblePage();

	wxRect pageRect = m_pageRects[pageIndex];
	int scrollTop = pageRect.GetTop() - m_pagePadding / 2;
	int pixelsPerUnitY;
	m_ctrl->GetScrollPixelsPerUnit(NULL, &pixelsPerUnitY);
	int scrollPosY = (scrollTop * m_ctrl->GetScaleY()) / pixelsPerUnitY;
	m_ctrl->Scroll(-1, scrollPosY);
}

void wxPDFViewImpl::GoToPage(int pageIndex)
{
	GoToPage(pageIndex, NULL);
}

void wxPDFViewImpl::GoToRemote(const wxString& path)
{
	wxCommandEvent gotoEvt(wxEVT_PDFVIEW_REMOTE_GOTO);
	gotoEvt.SetString(path);
	m_ctrl->AddPendingEvent(gotoEvt);
}

void wxPDFViewImpl::DoGoToAction(int pageIndex)
{
	CallAfter(&wxPDFViewImpl::GoToPage, pageIndex);
}

void wxPDFViewImpl::UpdateVirtualSize()
{
	int scrollSizeX = m_docSize.x * m_ctrl->GetScaleX();
	m_ctrl->SetVirtualSize(scrollSizeX, m_docSize.y * m_ctrl->GetScaleY());
	m_ctrl->SetScrollRate(wxRound(m_scrollStepX * m_ctrl->GetScaleX()), wxRound(m_scrollStepY * m_ctrl->GetScaleY()));
	int pixelsPerUnitX;
	m_ctrl->GetScrollPixelsPerUnit(&pixelsPerUnitX, NULL);
	wxSize clientSize = m_ctrl->GetClientSize();
	int scrollPosX = (scrollSizeX - clientSize.x) / 2;
	scrollPosX /= pixelsPerUnitX;
	m_ctrl->Scroll(scrollPosX, -1);
}

void wxPDFViewImpl::SetZoom(double zoom)
{
	if (zoom < m_minZoom)
		zoom = m_minZoom;
	else if (zoom > m_maxZoom)
		zoom = m_maxZoom;

	if (zoom == m_zoom)
		return;

	m_zoom = zoom;

	m_ctrl->SetScale(m_zoom, m_zoom);

	UpdateVirtualSize();
	AlignPageRects();
	CalcVisiblePages();
	m_ctrl->Refresh();
	wxCommandEvent zoomEvent(wxEVT_PDFVIEW_ZOOM_CHANGED);
	m_ctrl->ProcessEvent(zoomEvent);
}

void wxPDFViewImpl::SetZoomType(wxPDFViewZoomType zoomType)
{
	if (m_zoomType == zoomType)
		return;

	m_zoomType = zoomType;
	RecalculatePageRects();
	CalcZoomLevel();
	CalcVisiblePages();
	m_ctrl->Refresh();
	wxCommandEvent zoomEvent(wxEVT_PDFVIEW_ZOOM_TYPE_CHANGED);
	m_ctrl->ProcessEvent(zoomEvent);
}

void wxPDFViewImpl::SetDisplayFlags(int flags)
{
	if (m_displayFlags == flags)
		return;

	m_displayFlags = flags;
	RecalculatePageRects();
	CalcZoomLevel();
	CalcVisiblePages();
	m_ctrl->Refresh();
}

int wxPDFViewImpl::GetDisplayFlags() const
{
	return m_displayFlags;
}

void wxPDFViewImpl::StopFind()
{
	m_selection.clear();
	m_findResults.clear();
	m_nextPageToSearch = -1;
	m_lastPageToSearch = -1;
	m_lastCharacterIndexToSearch = -1;
	m_currentFindIndex = -1;
	m_findText.clear();
	m_ctrl->Refresh();
}

long wxPDFViewImpl::Find(const wxString& text, int flags)
{
	if (m_pages.empty())
		return wxNOT_FOUND;

	bool firstSearch = false;
	int characterToStartSearchingFrom = 0;
	if (m_findText != text) // First time we search for this text.
	{  
		firstSearch = true;
		wxVector<wxPDFViewTextRange> oldSelection = m_selection;
		StopFind();
		m_findText = text;

		if (m_findText.empty())
			return wxNOT_FOUND;

		if (oldSelection.empty()) {
			// Start searching from the beginning of the document.
			m_nextPageToSearch = -1;
			m_lastPageToSearch = GetPageCount() - 1;
			m_lastCharacterIndexToSearch = -1;
		} else {
			// There's a current selection, so start from it.
			m_nextPageToSearch = oldSelection[0].GetPage()->GetIndex();
			m_lastCharacterIndexToSearch = oldSelection[0].GetCharIndex();
			characterToStartSearchingFrom = oldSelection[0].GetCharIndex();
			m_lastPageToSearch = m_nextPageToSearch;
		}
	}

	if (m_findText.empty())
		return wxNOT_FOUND;

	bool caseSensitive = flags & wxPDFVIEW_FIND_MATCH_CASE;
	bool forward = (flags & wxPDFVIEW_FIND_BACKWARDS) == 0;

	// Move the find index
	if (forward)
		++m_currentFindIndex;
	else
		--m_currentFindIndex;

	// Determine if we need more results
	bool needMoreResults = true;
	if (m_currentFindIndex == static_cast<int>(m_findResults.size()))
		m_nextPageToSearch++;
	else if (m_currentFindIndex < 0)
		m_nextPageToSearch--;
	else
		needMoreResults = false;

	while (needMoreResults 
		&& m_nextPageToSearch < GetPageCount()
		&& m_nextPageToSearch >= 0)
	{
		int resultCount = FindOnPage(m_nextPageToSearch, caseSensitive, firstSearch, characterToStartSearchingFrom);
		if (resultCount)
			needMoreResults = false;
		else if (forward)
			++m_nextPageToSearch;
		else
			--m_nextPageToSearch;
	}

	if (m_findResults.empty())
		return wxNOT_FOUND;

	// Wrap find index
	if (m_currentFindIndex < 0)
		m_currentFindIndex = m_findResults.size() - 1;
	else if (m_currentFindIndex >= (int) m_findResults.size())
		m_currentFindIndex = 0;

	// Select result
	m_selection.clear();
	wxPDFViewTextRange result = m_findResults[m_currentFindIndex];
	m_selection.push_back(result);
	int resultPageIndex = result.GetPage()->GetIndex();
	// Make selection visible
	if (!IsPageVisible(resultPageIndex))
		GoToPage(resultPageIndex); // TODO: center selection rect
	else
		m_ctrl->Refresh();

	return m_findResults.size();
}

int wxPDFViewImpl::FindOnPage(int pageIndex, bool caseSensitive, bool firstSearch, int WXUNUSED(characterToStartSearchingFrom))
{
	// Find all the matches in the current page.
	unsigned long flags = caseSensitive ? FPDF_MATCHCASE : 0;
	wxMBConvUTF16LE conv;
	FPDF_SCHHANDLE find = FPDFText_FindStart(
		m_pages[pageIndex].GetTextPage(),
#ifdef __WXMSW__
		reinterpret_cast<FPDF_WIDESTRING>(m_findText.wc_str(conv)),
#else
		reinterpret_cast<FPDF_WIDESTRING>((const char*)m_findText.mb_str(conv)),
#endif
		flags, 0);

	wxPDFViewPage& page = m_pages[pageIndex];

	int resultCount = 0;
	while (FPDFText_FindNext(find)) 
	{
		wxPDFViewTextRange result(&page,
			FPDFText_GetSchResultIndex(find),
			FPDFText_GetSchCount(find));

		if (!firstSearch &&
			m_lastCharacterIndexToSearch != -1 &&
			result.GetPage()->GetIndex() == m_lastPageToSearch &&
			result.GetCharIndex() >= m_lastCharacterIndexToSearch)
		{
		  break;
		}

		AddFindResult(result);
		++resultCount;
	}

	FPDFText_FindClose(find);

	return resultCount;
}

void wxPDFViewImpl::AddFindResult(const wxPDFViewTextRange& result)
{
	// Figure out where to insert the new location, since we could have
	// started searching midway and now we wrapped.
	size_t i;
	int pageIndex = result.GetPage()->GetIndex();
	int charIndex = result.GetCharIndex();
	for (i = 0; i < m_findResults.size(); ++i) 
	{
		if (m_findResults[i].GetPage()->GetIndex() > pageIndex ||
			(m_findResults[i].GetPage()->GetIndex() == pageIndex &&
			m_findResults[i].GetCharIndex() > charIndex)) 
		{
			break;
		}
	}
	m_findResults.insert(m_findResults.begin() + i, result);
}

bool wxPDFViewImpl::IsPrintAllowed() const
{
	if (m_printValidator)
	{
		switch (m_printValidator->GetPrintPermission())
		{
			case wxPDFViewPrintValidator::Print_Allow:
				return true;
			case wxPDFViewPrintValidator::Print_Deny:
				return false;
			default:
				break;
		}
	}
	
	return (m_docPermissions & PDF_PERMISSION_PRINT_LOW_QUALITY) ||
		((m_docPermissions & PDF_PERMISSION_PRINT_HIGH_QUALITY) &&
		 (m_docPermissions & PDF_PERMISSION_PRINT_LOW_QUALITY));
}

wxPrintout* wxPDFViewImpl::CreatePrintout() const
{
	if (IsPrintAllowed())
	{
		bool forceBitmapPrintout = (m_docPermissions & PDF_PERMISSION_PRINT_HIGH_QUALITY) == 0;
		return new wxPDFViewPrintout(m_ctrl, forceBitmapPrintout);
	}
	else
		return NULL;
}

wxPrintDialogData wxPDFViewImpl::GetPrintDialogData() const
{
	wxPrintDialogData printDialogData;
	printDialogData.SetMinPage(1);
	printDialogData.SetMaxPage(GetPageCount());
	printDialogData.SetFromPage(1);
	printDialogData.SetToPage(GetPageCount());
	printDialogData.SetAllPages(true);
	
	if (m_printValidator)
		m_printValidator->PreparePrintDialogData(printDialogData);
	
	return printDialogData;
}

void wxPDFViewImpl::Print()
{
	wxPrintDialogData printDialogData = GetPrintDialogData();
	wxPrinter printer(&printDialogData);
	wxSharedPtr<wxPrintout> printout(CreatePrintout());
	if (!printer.Print(GetCtrl(), printout.get()))
	{
		if (printer.GetLastError() == wxPRINTER_ERROR)
			wxLogError(_("Document could not be printed"));
	}
}

const wxString& wxPDFViewImpl::GetDocumentTitle() const
{
	return m_documentTitle;
}

bool wxPDFViewImpl::LoadStream(wxSharedPtr<std::istream> pStream, const wxString& password)
{
	CloseDocument();

	m_pDataStream = pStream;

	// Determine file size
	pStream->seekg(0, std::ios::end);
	unsigned long docFileSize = pStream->tellg();
	pStream->seekg(0);

	memset(&m_platformCallbacks, '\0', sizeof(m_platformCallbacks));
	m_platformCallbacks.version = 3;
	m_platformCallbacks.app_alert = &Form_Alert;
	m_platformCallbacks.Doc_gotoPage = &Form_GotoPage;
	m_platformCallbacks.Doc_print = &Form_Print;
	m_platformCallbacks.Field_browse = &Form_Browse;
	m_platformCallbacks.m_pFormfillinfo = &m_formCallbacks;
#ifdef PDFIUM_HAVE_JS_APP_OPENDOC
	m_platformCallbacks.version = 4;
	m_platformCallbacks.app_openDoc = &Form_OpenDoc;
#endif

	memset(&m_formCallbacks, '\0', sizeof(m_formCallbacks));
	m_formCallbacks.version = 1;
	m_formCallbacks.FFI_SetCursor = &FFI_SetCursor;
	m_formCallbacks.FFI_Invalidate = &FFI_Invalidate;
	m_formCallbacks.FFI_GetCurrentPage = &FFI_GetCurrentPage;
	m_formCallbacks.FFI_GetPage = &FFI_GetPage;
	m_formCallbacks.FFI_DoGoToAction = &FFI_DoGoToAction;
	m_formCallbacks.FFI_ExecuteNamedAction = &FFI_ExecuteNamedAction;
	m_formCallbacks.m_pJsPlatform = &m_platformCallbacks;

	m_pdfFileAccess.m_FileLen = docFileSize;

	m_pdfAvail = FPDFAvail_Create(&m_pdfFileAvail, &m_pdfFileAccess);

	(void) FPDFAvail_IsDocAvail(m_pdfAvail, &m_hints);

	bool retryLoad = true;

	wxString loadPassword = (password.empty()) ? m_prevLoadPassword : password;
	while (retryLoad)
	{
		g_unsupportedHandlerPDFViewImpl = this;
		FPDF_BYTESTRING pdfPassword = loadPassword.c_str();
		m_linearized = FPDFAvail_IsLinearized(m_pdfAvail) != 0;
		if (!m_linearized)
			m_pdfDoc = FPDF_LoadCustomDocument(&m_pdfFileAccess, pdfPassword);
		else
			m_pdfDoc = FPDFAvail_GetDocument(m_pdfAvail, pdfPassword);
		g_unsupportedHandlerPDFViewImpl = NULL;

		if (!m_pdfDoc)
		{
			unsigned long error = FPDF_GetLastError();
			if (error == FPDF_ERR_PASSWORD)
			{
				wxPasswordEntryDialog dlg(m_ctrl, _("Password to open the document"));
				dlg.SetValue(loadPassword);
				if (dlg.ShowModal() == wxID_OK)
				{
					loadPassword = dlg.GetValue();
					m_prevLoadPassword = loadPassword;
					continue;
				}
			}

			LogPDFError(error);
			return false;
		}
		retryLoad = false;
	}

	m_docPermissions = FPDF_GetDocPermissions(m_pdfDoc);
	(void) FPDFAvail_IsFormAvail(m_pdfAvail, &m_hints);

	m_pdfForm = FPDFDOC_InitFormFillEnvironment(m_pdfDoc, &m_formCallbacks);
	
	g_pdfFormMap[&m_formCallbacks] = this;

	int first_page = FPDFAvail_GetFirstPageNum(m_pdfDoc);
	(void) FPDFAvail_IsPageAvail(m_pdfAvail, first_page, &m_hints);

	m_pages.SetDocument(m_pdfDoc);
	m_pages.SetForm(m_pdfForm);
	
	RecalculatePageRects();

	if (GetPageCount() > 0)
		GetCachedBitmap(0, m_pageRects[0].GetSize());

	m_bookmarks = new wxPDFViewBookmarks(m_pdfDoc);
	m_ctrl->Refresh();
	UpdateDocumentInfo();

	FORM_DoDocumentJSAction(m_pdfForm);
	FORM_DoDocumentOpenAction(m_pdfForm);

	return true;
}

void wxPDFViewImpl::CloseDocument()
{
	wxCommandEvent readyEvent(wxEVT_PDFVIEW_DOCUMENT_CLOSED);
	m_ctrl->ProcessEvent(readyEvent);

	m_pages.SetDocument(NULL);
	if (m_pdfForm)
	{
		FORM_DoDocumentAAction(m_pdfForm, FPDFDOC_AACTION_WC);
		FPDFDOC_ExitFormFillEnvironment(m_pdfForm);
		m_pdfForm = NULL;
	}
	if (m_pdfDoc)
	{
		FPDF_CloseDocument(m_pdfDoc);
		m_pdfDoc = NULL;
	}
	if (m_pdfAvail)
	{
		FPDFAvail_Destroy(m_pdfAvail);
		m_pdfAvail = NULL;
	}
	wxDELETE(m_bookmarks);
	m_pageRects.clear();
	m_docSize = wxDefaultSize;
	UpdateVirtualSize();
	m_docPermissions = 0;
	m_mostVisiblePage = -1;

	ClearBitmapCache();
}

void wxPDFViewImpl::HandleScrollWindow(int WXUNUSED(dx), int WXUNUSED(dy))
{
	CalcVisiblePages();
}

wxRect wxPDFViewImpl::UnscaledToScaled(const wxRect& rect) const
{
	wxRect scaledRect;
	scaledRect.x = rect.x * m_ctrl->GetScaleX();
	scaledRect.y = rect.y * m_ctrl->GetScaleY();
	scaledRect.width = rect.width * m_ctrl->GetScaleX();
	scaledRect.height = rect.height * m_ctrl->GetScaleX();

	return scaledRect;
}

wxRect wxPDFViewImpl::ScaledToUnscaled(const wxRect& rect) const
{
	wxRect scaledRect;
	scaledRect.x = rect.x / m_ctrl->GetScaleX();
	scaledRect.y = rect.y / m_ctrl->GetScaleY();
	scaledRect.width = rect.width / m_ctrl->GetScaleX();
	scaledRect.height = rect.height / m_ctrl->GetScaleX();

	return scaledRect;
}

int wxPDFViewImpl::ClientToPage(const wxPoint& clientPos, wxPoint& pagePos)
{
	if (m_pages.empty())
		return -1;

	wxPoint docPos = m_ctrl->CalcUnscrolledPosition(clientPos);
	docPos.x /= m_ctrl->GetScaleX();
	docPos.y /= m_ctrl->GetScaleY();

	for (int i = GetFirstVisiblePage(); i <= GetLastVisiblePage(); ++i)
	{
		wxRect pageRect = m_pageRects[i];

		if (pageRect.Contains(docPos))
		{
			pagePos.x = docPos.x - pageRect.x;
			pagePos.y = docPos.y - pageRect.y;
			return i;
		}
		else if (pageRect.y + pageRect.height > docPos.y)
			break;
	}

	return -1;
}

bool wxPDFViewImpl::EvaluateLinkTargetPageAtClientPos(const wxPoint& clientPos, int evtType)
{
	bool foundLink = false;

	wxPoint pagePos;
	int pageIndex = ClientToPage(clientPos, pagePos);
	if (pageIndex >= 0)
	{
		FPDF_PAGE page = m_pages[pageIndex].GetPage();
		wxRect pageRect = m_pageRects[pageIndex];
		double page_x;
		double page_y;
		FPDF_DeviceToPage(page, 0, 0, pageRect.width, pageRect.height, 0, pagePos.x, pagePos.y, &page_x, &page_y);
		FPDF_LINK link = FPDFLink_GetLinkAtPoint(page, page_x, page_y);
		if (link)
		{
			foundLink = true;
			if (evtType == wxEVT_LEFT_UP)
			{
				FPDF_DEST dest = FPDFLink_GetDest(m_pdfDoc, link);
				if (!dest)
				{
					FPDF_ACTION action = FPDFLink_GetAction(link);
					if (action)
					{
						unsigned long actionType = FPDFAction_GetType(action);
						switch (actionType)
						{
							case PDFACTION_GOTO:
								dest = FPDFAction_GetDest(m_pdfDoc, action);
								break;
							case PDFACTION_URI:
								{
									int uriSize = FPDFAction_GetURIPath(m_pdfDoc, action, NULL, 0);
									char* uriBuf = new char[uriSize];
									FPDFAction_GetURIPath(m_pdfDoc, action, uriBuf, uriSize);
									wxString uriString(uriBuf, uriSize - 1);

									wxCommandEvent urlEvent(wxEVT_PDFVIEW_URL_CLICKED);
									urlEvent.SetString(uriString);
									m_ctrl->ProcessEvent(urlEvent);

									delete [] uriBuf;
									break;
								}
							case PDFACTION_REMOTEGOTO:
							{
								char cpath[512];
								FPDFAction_GetFilePath(action, cpath, 512);
								GoToRemote(wxString::FromUTF8(cpath));
								break;
							}
							case PDFACTION_LAUNCH:
							{
								char cpath[512];
								FPDFAction_GetFilePath(action, cpath, 512);
								wxString path = wxString::FromUTF8(cpath);
								path.Replace(":", "/");
								wxFileName fn(path);
								if (fn.GetExt().IsSameAs("pdf", false))
								{
									// Handle like a remote goto
									GoToRemote(path);
								} else {
									wxCommandEvent gotoEvt(wxEVT_PDFVIEW_LAUNCH);
									gotoEvt.SetString(path);
									m_ctrl->ProcessEvent(gotoEvt);
								}
								
								break;
							}
						}
					}
				}

				if (dest)
					GoToPage(FPDFDest_GetPageIndex(m_pdfDoc, dest));
			}
		}
		else
		{
			if (evtType == wxEVT_LEFT_UP)
				FORM_OnLButtonUp(m_pdfForm, page, 0, page_x, page_y);
			else if (evtType == wxEVT_LEFT_DOWN)
				FORM_OnLButtonDown(m_pdfForm, page, 0, page_x, page_y);
			if (evtType == wxEVT_MOTION)
				FORM_OnMouseMove(m_pdfForm, page, 0, page_x, page_y);
		}
	}

	return foundLink;
}

void wxPDFViewImpl::CalcVisiblePages()
{
	wxRect viewRect(m_ctrl->CalcUnscrolledPosition(wxPoint(0, 0)), m_ctrl->GetClientSize());
	viewRect = ScaledToUnscaled(viewRect);

	int firstPage = -1;
	int lastPage = -1;

	int pageIndex = 0;
	for (auto it = m_pageRects.begin(); it != m_pageRects.end(); ++it, ++pageIndex)
	{
		if (it->Intersects(viewRect))
		{
			if (firstPage == -1)
				firstPage = pageIndex;
			if (pageIndex > lastPage)
				lastPage = pageIndex;
		}

		if (it->y > viewRect.y + viewRect.height)
			break;
	}

	int newMostVisiblePage = firstPage;
	if (GetPagePosition(newMostVisiblePage) == wxPDFVIEW_PAGE_POS_LEFT)
		newMostVisiblePage++;
	// Check if we can see more of the next page than the first one
	if (newMostVisiblePage >= 0 && 
		newMostVisiblePage < GetPageCount() - 1 &&
		newMostVisiblePage < lastPage)
	{
		const wxRect cviewRect = viewRect;
		wxRect firstRect = cviewRect.Intersect(m_pageRects[newMostVisiblePage]);
		wxRect nextRect = cviewRect.Intersect(m_pageRects[newMostVisiblePage + 1]);
		if (nextRect.GetHeight() > firstRect.GetHeight())
			newMostVisiblePage++;
	}

	if (newMostVisiblePage >= 0 && newMostVisiblePage != m_mostVisiblePage)
	{
		if (m_mostVisiblePage >= 0)
			FORM_DoPageAAction(m_pages[m_mostVisiblePage].GetPage(), m_pdfForm, FPDFPAGE_AACTION_CLOSE);

		if (GetPagePosition(newMostVisiblePage) == wxPDFVIEW_PAGE_POS_RIGHT && newMostVisiblePage > 0)
			newMostVisiblePage--;

		m_mostVisiblePage = newMostVisiblePage;
		FORM_DoPageAAction(m_pages[m_mostVisiblePage].GetPage(), m_pdfForm, FPDFPAGE_AACTION_OPEN);

		wxCommandEvent* evt = new wxCommandEvent(wxEVT_PDFVIEW_PAGE_CHANGED);
		evt->SetInt(m_mostVisiblePage);
		m_ctrl->QueueEvent(evt);
	}

	if (firstPage != GetFirstVisiblePage() || lastPage != GetLastVisiblePage())
		SetVisiblePages(firstPage, lastPage);
}

void wxPDFViewImpl::CalcZoomLevel()
{
	if (m_pages.empty() || m_mostVisiblePage < 0)
		return;

	wxSize clientSize = m_ctrl->GetClientSize();

	double scale = 0;

	switch (m_zoomType)
	{
		case wxPDFVIEW_ZOOM_TYPE_PAGE_WIDTH:
			scale = (double) clientSize.x / (double) (m_docSize.x + 6);
			break;
		case wxPDFVIEW_ZOOM_TYPE_FIT_PAGE:
		{
			wxSize pageSize(m_docSize.x, m_maxPageHeight);
			pageSize.x += 6; // Add padding to page width
			pageSize.y += m_pagePadding;
			if (pageSize.x > clientSize.x)
			{
				scale = (double) clientSize.x / (double) pageSize.x;
			} else {
				scale = (double) clientSize.y / (double) pageSize.y;
			}
			break;
		}
		case wxPDFVIEW_ZOOM_TYPE_FREE:
			break;
	}

	if (scale > 0)
		SetZoom(scale);
}

wxSize wxPDFViewImpl::GetPageSize(int pageIndex) const
{
	return m_pageRects[pageIndex].GetSize();
}

wxPDFViewPagePosition wxPDFViewImpl::GetPagePosition(int pageIndex) const
{
	int pageOffset;
	if (m_displayFlags & wxPDFVIEW_DISPLAY_TWO_PAGE_COVER)
		pageOffset = 1;
	else
		pageOffset = 0;

	if (GetPageCount() > 1 &&
		(m_displayFlags & wxPDFVIEW_DISPLAY_TWO_PAGE) &&
		!(m_displayFlags & wxPDFVIEW_DISPLAY_TWO_PAGE_COVER && pageIndex == 0))
	{
		if ((pageIndex + pageOffset) % 2 == 0)
			return wxPDFVIEW_PAGE_POS_LEFT;
		else
			return wxPDFVIEW_PAGE_POS_RIGHT;
	}
	else
		return wxPDFVIEW_PAGE_POS_CENTER;
}

void wxPDFViewImpl::RefreshPage(int pageIndex)
{
	if (IsPageVisible(pageIndex))
	{
		wxRect updateRect = m_pageRects[pageIndex];
		updateRect = UnscaledToScaled(updateRect);
		updateRect.SetPosition(m_ctrl->CalcScrolledPosition(updateRect.GetPosition()));

		m_ctrl->RefreshRect(updateRect, true);
	}
}

void wxPDFViewImpl::InvalidatePage(int pageIndex, const wxRect* WXUNUSED(rect))
{
	RemoveCachedBitmap(pageIndex);
	RefreshPage(pageIndex);
}

void wxPDFViewImpl::SetDefaultCursor(wxStockCursor cursor)
{
	m_defaultCursor = cursor;
	m_ctrl->SetCursor(m_defaultCursor);
}

void wxPDFViewImpl::ExecuteNamedAction(const wxString& action)
{
	wxCommandEvent evt(wxEVT_PDFVIEW_NAMED_ACTION);
	evt.SetString(action);
	if (m_ctrl->ProcessEvent(evt))
		return;

	if (action.IsSameAs("Print", false))
	{
		if (IsPrintAllowed())
			CallAfter(&wxPDFViewImpl::Print);
	}
	else if (action.IsSameAs("NextPage", false))
	{
		CallAfter(&wxPDFViewImpl::NavigateToPage, wxPDFVIEW_PAGE_NAV_NEXT);
	}
	else if (action.IsSameAs("PrevPage", false))
	{
		CallAfter(&wxPDFViewImpl::NavigateToPage, wxPDFVIEW_PAGE_NAV_PREV);
	}
	else if (action.IsSameAs("GoBack", false))
	{
		if (m_backPage >= 0)
			CallAfter(&wxPDFViewImpl::GoToPage, m_backPage);
	}
	else if (action.IsSameAs("FirstPage", false))
	{
		CallAfter(&wxPDFViewImpl::GoToPage, 0);
	}
	else
		wxLogDebug("Unhandled action: %s", action);
}

void wxPDFViewImpl::HandleUnsupportedFeature(int type)
{
	wxString feature = "Unknown";
	switch (type) {
		case FPDF_UNSP_DOC_XFAFORM:
			feature = "XFA";
			break;
		case FPDF_UNSP_DOC_PORTABLECOLLECTION:
			feature = "Portfolios_Packages";
			break;
		case FPDF_UNSP_DOC_ATTACHMENT:
		case FPDF_UNSP_ANNOT_ATTACHMENT:
			feature = "Attachment";
			break;
		case FPDF_UNSP_DOC_SECURITY:
			feature = "Rights_Management";
			break;
		case FPDF_UNSP_DOC_SHAREDREVIEW:
			feature = "Shared_Review";
			break;
		case FPDF_UNSP_DOC_SHAREDFORM_ACROBAT:
		case FPDF_UNSP_DOC_SHAREDFORM_FILESYSTEM:
		case FPDF_UNSP_DOC_SHAREDFORM_EMAIL:
			feature = "Shared_Form";
			break;
		case FPDF_UNSP_ANNOT_3DANNOT:
			feature = "3D";
			break;
		case FPDF_UNSP_ANNOT_MOVIE:
			feature = "Movie";
			break;
		case FPDF_UNSP_ANNOT_SOUND:
			feature = "Sound";
			break;
		case FPDF_UNSP_ANNOT_SCREEN_MEDIA:
		case FPDF_UNSP_ANNOT_SCREEN_RICHMEDIA:
			feature = "Screen";
			break;
		case FPDF_UNSP_ANNOT_SIG:
			feature = "Digital_Signature";
			break;
	}
	
	wxCommandEvent unsuppEvent(wxEVT_PDFVIEW_UNSUPPORTED_FEATURE);
	unsuppEvent.SetString(feature);
	unsuppEvent.SetInt(type);
	m_ctrl->ProcessEvent(unsuppEvent);
}

void wxPDFViewImpl::SendActivity(const wxString& description)
{
	wxCommandEvent evt(wxEVT_PDFVIEW_ACTIVITY);
	evt.SetString(description);
	m_ctrl->ProcessEvent(evt);
}

bool wxPDFViewImpl::AcquireSDK()
{
	if (ms_pdfSDKRefCount == 0)
	{
		// Initialize PDF Rendering library
		if (!ms_v8initialized)
		{
			v8::V8::InitializeICU();
			
			v8::Platform* platform = v8::platform::CreateDefaultPlatform();
			v8::V8::InitializePlatform(platform);
			v8::V8::Initialize();
			
			// By enabling predictable mode, V8 won't post any background tasks.
			const char predictable_flag[] = "--predictable";
			v8::V8::SetFlagsFromString(predictable_flag,
									   static_cast<int>(strlen(predictable_flag)));
			
			ms_v8initialized = true;
		}
		
		FPDF_LIBRARY_CONFIG config;
		config.version = 2;
		config.m_pUserFontPaths = NULL;
		config.m_pIsolate = NULL;
		config.m_v8EmbedderSlot = 0;
		FPDF_InitLibraryWithConfig(&config);

		FSDK_SetUnSpObjProcessHandler(&g_unsupported_info);
	}
	wxAtomicInc(ms_pdfSDKRefCount);
	return true;
}

void wxPDFViewImpl::ReleaseSDK()
{
	wxAtomicDec(ms_pdfSDKRefCount);
	if (ms_pdfSDKRefCount == 0)
	{
		FPDF_DestroyLibrary();
	}
}
