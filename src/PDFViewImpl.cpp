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
#include <wx/textdlg.h>

#include <v8.h>
#include "fpdf_ext.h"
#include "fpdftext.h"

// See Table 3.20 in
// http://www.adobe.com/devnet/acrobat/pdfs/pdf_reference_1-7.pdf
#define PDF_PERMISSION_PRINT_LOW_QUALITY	1 << 2
#define PDF_PERMISSION_PRINT_HIGH_QUALITY	1 << 11
#define PDF_PERMISSION_COPY					1 << 4
#define PDF_PERMISSION_COPY_ACCESSIBLE		1 << 9

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

	wxString msgTitle = (wchar_t*) Title;
	wxString msgMsg = (wchar_t*) Msg;

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

void Unsupported_Handler(UNSUPPORT_INFO*, int type)
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
  wxLogError("Unsupported feature: %s.", feature.c_str());
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

bool Is_Data_Avail(FX_FILEAVAIL* WXUNUSED(pThis), size_t WXUNUSED(offset), size_t WXUNUSED(size))
{
  return true;
}

void Add_Segment(FX_DOWNLOADHINTS* WXUNUSED(pThis), size_t WXUNUSED(offset), size_t WXUNUSED(size))
{
}

//
// wxPDFViewImpl
//

wxAtomicInt wxPDFViewImpl::ms_pdfSDKRefCount = 0;

wxPDFViewImpl::wxPDFViewImpl(wxPDFView* ctrl):
	m_ctrl(ctrl),
	m_handCursor(wxCURSOR_HAND)
{
	AcquireSDK();

	SetPages(&m_pages);

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

	// PDF SDK structures
	memset(&m_pdfFileAccess, '\0', sizeof(m_pdfFileAccess));
	m_pdfFileAccess.m_FileLen = 0;
	m_pdfFileAccess.m_GetBlock = Get_Block;
	m_pdfFileAccess.m_Param = this;

	memset(&m_pdfFileAvail, '\0', sizeof(m_pdfFileAvail));
	m_pdfFileAvail.version = 1;
	m_pdfFileAvail.IsDataAvail = Is_Data_Avail;

	m_ctrl->Bind(wxEVT_PAINT, &wxPDFViewImpl::OnPaint, this);
	m_ctrl->Bind(wxEVT_SIZE, &wxPDFViewImpl::OnSize, this);
	m_ctrl->Bind(wxEVT_MOUSEWHEEL, &wxPDFViewImpl::OnMouseWheel, this);
	m_ctrl->Bind(wxEVT_MOTION, &wxPDFViewImpl::OnMouseMotion, this);
	m_ctrl->Bind(wxEVT_LEFT_UP, &wxPDFViewImpl::OnMouseLeftUp, this);
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
			GoToPage(GetMostVisiblePage() + 1);
			break;
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
	m_pageRects.reserve(GetPageCount());
	
	m_docSize.Set(0, 0);
	wxRect pageRect;
	for (int i = 0; i < GetPageCount(); ++i)
	{
		double width;
		double height;
		if (FPDF_GetPageSizeByIndex(m_pdfDoc, i, &width, &height))
		{
			pageRect.y += m_pagePadding / 2;
			pageRect.width = width;
			pageRect.height = height;
			m_pageRects.push_back(pageRect);
			
			if (width > m_docSize.x)
				m_docSize.x = width;
			
			pageRect.y += height;
			pageRect.y += m_pagePadding / 2;
			
			// Make sure every page top is pixel exact scrollable
			int pageDisplayHeight = pageRect.height + m_pagePadding;
			int scrollMod = pageDisplayHeight % m_scrollStepY;
			if (scrollMod)
				pageRect.y += m_scrollStepY - scrollMod;
		} else {
			// Document broken?
			m_pages.erase(m_pages.begin() + i, m_pages.end());
			break;
		}
	}
	m_docSize.SetHeight(pageRect.y - (m_pagePadding / 2));
	
	AlignPageRects();
}

void wxPDFViewImpl::AlignPageRects()
{
	int ctrlWidth = m_ctrl->GetVirtualSize().GetWidth() / m_ctrl->GetScaleX();
	for (auto it = m_pageRects.begin(); it != m_pageRects.end(); ++it)
		it->x = (ctrlWidth - it->width) / 2;
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
			m_pages[pageIndex].Draw(this, dc, *gc, pageRect);
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
	if (EvaluateLinkTargetPageAtClientPos(event.GetPosition(), false))
		m_ctrl->SetCursor(m_handCursor);
	else
		m_ctrl->SetCursor(wxCURSOR_ARROW);

	event.Skip();
}

void wxPDFViewImpl::OnMouseLeftUp(wxMouseEvent& event)
{
	if (EvaluateLinkTargetPageAtClientPos(event.GetPosition(), true))
		m_ctrl->SetCursor(wxCURSOR_ARROW);

	event.Skip();
}

void wxPDFViewImpl::GoToPage(int pageIndex, const wxRect* centerRect)
{
	if (pageIndex < 0)
		pageIndex = 0;
	else if (pageIndex >= GetPageCount())
		pageIndex = GetPageCount() - 1;

	wxRect pageRect = m_pageRects[pageIndex];
	int scrollTop = pageRect.GetTop() - m_pagePadding / 2;
	int pixelsPerUnitY;
	m_ctrl->GetScrollPixelsPerUnit(NULL, &pixelsPerUnitY);
	int scrollPosY = (scrollTop * m_ctrl->GetScaleY()) / pixelsPerUnitY;
	m_ctrl->Scroll(-1, scrollPosY);
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
	CalcZoomLevel();
	wxCommandEvent zoomEvent(wxEVT_PDFVIEW_ZOOM_TYPE_CHANGED);
	m_ctrl->ProcessEvent(zoomEvent);
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
		m_nextPageToSearch = m_mostVisiblePage + 1;
	else if (m_currentFindIndex < 0)
		m_nextPageToSearch = m_mostVisiblePage - 1;
	else
		needMoreResults = false;

	bool endOfSearch = false;

	while (needMoreResults && !endOfSearch)
	{
		int resultCount = FindOnPage(m_nextPageToSearch, caseSensitive, firstSearch, characterToStartSearchingFrom);
		if (resultCount)
			needMoreResults = false;
		else if (forward)
			++m_nextPageToSearch;
		else
			--m_nextPageToSearch;

		if (m_nextPageToSearch == GetPageCount())
			endOfSearch = true;
	}

	if (endOfSearch || m_findResults.empty())
		return wxNOT_FOUND;

	//TODO: Wrap find index
	if (m_currentFindIndex < 0)
		m_currentFindIndex = m_findResults.size() - 1;

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
		reinterpret_cast<FPDF_WIDESTRING>((const char*)m_findText.mb_str(conv)),
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
	return (m_docPermissions & PDF_PERMISSION_PRINT_HIGH_QUALITY) ||
		(m_docPermissions & PDF_PERMISSION_PRINT_LOW_QUALITY);
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
	return printDialogData;
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

	IPDF_JSPLATFORM platform_callbacks;
	memset(&platform_callbacks, '\0', sizeof(platform_callbacks));
	platform_callbacks.version = 1;
	platform_callbacks.app_alert = Form_Alert;

	FPDF_FORMFILLINFO form_callbacks;
	memset(&form_callbacks, '\0', sizeof(form_callbacks));
	form_callbacks.version = 1;
	form_callbacks.m_pJsPlatform = &platform_callbacks;

	m_pdfFileAccess.m_FileLen = docFileSize;

	FX_DOWNLOADHINTS hints;
	memset(&hints, '\0', sizeof(hints));
	hints.version = 1;
	hints.AddSegment = Add_Segment;

	m_pdfAvail = FPDFAvail_Create(&m_pdfFileAvail, &m_pdfFileAccess);

	(void) FPDFAvail_IsDocAvail(m_pdfAvail, &hints);

	bool retryLoad = true;

	wxString loadPassword = password;
	while (retryLoad)
	{
		FPDF_BYTESTRING pdfPassword = loadPassword.c_str();
		if (!FPDFAvail_IsLinearized(m_pdfAvail))
			m_pdfDoc = FPDF_LoadCustomDocument(&m_pdfFileAccess, pdfPassword);
		else
			m_pdfDoc = FPDFAvail_GetDocument(m_pdfAvail, pdfPassword);

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
					continue;
				}
			}

			LogPDFError(error);
			return false;
		}
		retryLoad = false;
	}

	m_docPermissions = FPDF_GetDocPermissions(m_pdfDoc);
	(void) FPDFAvail_IsFormAvail(m_pdfAvail, &hints);

	m_pdfForm = FPDFDOC_InitFormFillEnviroument(m_pdfDoc, &form_callbacks);
	FPDF_SetFormFieldHighlightColor(m_pdfForm, 0, 0xFFE4DD);
	FPDF_SetFormFieldHighlightAlpha(m_pdfForm, 100);

	int first_page = FPDFAvail_GetFirstPageNum(m_pdfDoc);
	(void) FPDFAvail_IsPageAvail(m_pdfAvail, first_page, &hints);

	m_pages.SetDocument(m_pdfDoc);
	
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
	m_pages.SetDocument(NULL);
	if (m_pdfForm)
	{
		FORM_DoDocumentAAction(m_pdfForm, FPDFDOC_AACTION_WC);
		FPDFDOC_ExitFormFillEnviroument(m_pdfForm);
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

bool wxPDFViewImpl::EvaluateLinkTargetPageAtClientPos(const wxPoint& clientPos, bool performAction)
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
			if (performAction)
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
						}
					}
				}

				if (dest)
					GoToPage(FPDFDest_GetPageIndex(m_pdfDoc, dest));
			}
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
		m_mostVisiblePage = newMostVisiblePage;
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
			wxSize pageSize = m_pageRects[m_mostVisiblePage].GetSize();
			pageSize.x += 6; // Add padding to page width
			pageSize.y += m_pagePadding;
			if (pageSize.x > pageSize.y)
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

bool wxPDFViewImpl::AcquireSDK()
{
	if (ms_pdfSDKRefCount == 0)
	{
		// Initialize PDF Rendering library
		v8::V8::InitializeICU();

		FPDF_InitLibrary(NULL);

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
