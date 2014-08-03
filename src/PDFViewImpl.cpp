#include "PDFViewImpl.h"

#include <wx/webview.h>

#include <wx/dcbuffer.h>
#include <v8.h>
#include "fpdf_ext.h"
#include "fpdftext.h"

wxDECLARE_EVENT(wxEVT_BMP_CACHE_AVAILABLE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_BMP_CACHE_AVAILABLE, wxThreadEvent);

void LogPDFError()
{
	unsigned long error = FPDF_GetLastError();
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

int Form_Alert(IPDF_JSPLATFORM* pThis, FPDF_WIDESTRING Msg, FPDF_WIDESTRING Title, int Type, int Icon)
{
	wxLogDebug("Form_Alert called.");
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
  std::string feature = "Unknown";
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

bool Is_Data_Avail(FX_FILEAVAIL* pThis, size_t offset, size_t size)
{
  return true;
}

void Add_Segment(FX_DOWNLOADHINTS* pThis, size_t offset, size_t size)
{
}

//
// wxPDFViewImpl
//

wxPDFViewImpl::wxPDFViewImpl(wxPDFView* ctrl):
	m_ctrl(ctrl),
	m_handCursor(wxCURSOR_HAND)
{
	m_zoomType = wxPDFVIEW_ZOOM_TYPE_FREE;
	m_pagePadding = 16;
	m_scrollStepX = 20;
	m_scrollStepY = 20;
	m_zoom = 100;
	m_minZoom = 10;
	m_maxZoom = 800;
	m_pDataStream.reset();
	m_pdfDoc = NULL;
	m_pdfForm = NULL;
	m_pdfAvail = NULL;

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
	m_ctrl->Bind(wxEVT_BMP_CACHE_AVAILABLE, &wxPDFViewImpl::OnCacheBmpAvailable, this);
	m_ctrl->Bind(wxEVT_SCROLLWIN_LINEUP, &wxPDFViewImpl::OnScroll, this);
	m_ctrl->Bind(wxEVT_SCROLLWIN_LINEDOWN, &wxPDFViewImpl::OnScroll, this);
	m_ctrl->Bind(wxEVT_SCROLLWIN_PAGEUP, &wxPDFViewImpl::OnScroll, this);
	m_ctrl->Bind(wxEVT_SCROLLWIN_PAGEDOWN, &wxPDFViewImpl::OnScroll, this);
	m_ctrl->Bind(wxEVT_SCROLLWIN_TOP, &wxPDFViewImpl::OnScroll, this);
	m_ctrl->Bind(wxEVT_SCROLLWIN_BOTTOM, &wxPDFViewImpl::OnScroll, this);

	// Init bitmap request handler
	m_bmpRequestHandlerActive = true;
	m_bmpRequestHandlerCondition = NULL;

	if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)     
	{         
		wxLogError("Could not create the worker thread!");         
		return;     
	}

	if (GetThread()->Run() != wxTHREAD_NO_ERROR)
	{
		wxLogError("Could not run the worker thread!");
		return;
	}

	// Initialize PDF Rendering library
	v8::V8::InitializeICU();

	FPDF_InitLibrary(NULL);

	FSDK_SetUnSpObjProcessHandler(&g_unsupported_info);
}

wxPDFViewImpl::~wxPDFViewImpl()
{
	// End bitmap request handler
	m_bmpRequestHandlerActive = false;
	m_bmpRequestHandlerCondition->Signal();

	CloseDocument();

	FPDF_DestroyLibrary();
}

void wxPDFViewImpl::NavigateToPage(wxPDFViewPageNavigation pageNavigation)
{
	switch (pageNavigation)
	{
		case wxPDFVIEW_PAGE_NAV_NEXT:
			SetCurrentPage(GetCurrentPage() + 1);
			break;
		case wxPDFVIEW_PAGE_NAV_PREV:
			SetCurrentPage(GetCurrentPage() + 0);
			break;
		case wxPDFVIEW_PAGE_NAV_FIRST:
			SetCurrentPage(0);
			break;
		case wxPDFVIEW_PAGE_NAV_LAST:
			SetCurrentPage(GetPageCount() - 1);
			break;
	}
}

void wxPDFViewImpl::UpdateDocumentInfo()
{
	UpdateVirtualSize();

	m_ctrl->ProcessEvent(wxCommandEvent(wxEVT_PDFVIEW_DOCUMENT_READY));
	wxCommandEvent pgEvent(wxEVT_PDFVIEW_PAGE_CHANGED);
	pgEvent.SetInt(0);
	m_ctrl->ProcessEvent(pgEvent);
}

void wxPDFViewImpl::AlignPageRects()
{
	int ctrlWidth = m_ctrl->GetVirtualSize().GetWidth() / m_ctrl->GetScaleX();
	for (auto it = m_pageRects.begin(); it != m_pageRects.end(); ++it)
		it->x = (ctrlWidth - it->width) / 2;
}

bool wxPDFViewImpl::DrawPage(wxGraphicsContext& gc, int pageIndex, const wxRect& pageRect)
{
	// Draw page background
	wxRect bgRect = pageRect.Inflate(2, 2);
	gc.SetBrush(*wxWHITE_BRUSH);
	gc.SetPen(*wxBLACK_PEN);
	gc.DrawRectangle(bgRect.x, bgRect.y, bgRect.width, bgRect.height);

	wxSize bmpSize(m_pageRects[pageIndex].GetWidth() * m_ctrl->GetScaleX(), m_pageRects[pageIndex].GetHeight() * m_ctrl->GetScaleY());
	wxBitmap pageBmp;
	bool matchingBitmap = m_bitmapCache.GetBitmapForPage(pageIndex, bmpSize, pageBmp);
	if (pageBmp.IsOk())
		gc.DrawBitmap(pageBmp, pageRect.x, pageRect.y, pageRect.width, pageRect.height);
	return matchingBitmap;
}

void wxPDFViewImpl::RenderPage(int pageIndex)
{
	wxSize bmpSize(m_pageRects[pageIndex].width * m_ctrl->GetScaleX(), m_pageRects[pageIndex].height * m_ctrl->GetScaleY());
	wxLogDebug("Rendering page %d (%dx%d)...", pageIndex, bmpSize.x, bmpSize.y);

	m_bitmapCache.RenderPage(pageIndex, bmpSize, m_pdfDoc, m_pdfForm);

	wxThreadEvent evt(wxEVT_BMP_CACHE_AVAILABLE);
	evt.SetInt(pageIndex);
	m_ctrl->AddPendingEvent(evt);
}

void wxPDFViewImpl::OnCacheBmpAvailable(wxThreadEvent& event)
{
	wxRect updateRect = m_pageRects[event.GetInt()];
	updateRect = UnscaledToScaled(updateRect);
	updateRect.SetPosition(m_ctrl->CalcScrolledPosition(updateRect.GetPosition()));

	m_ctrl->RefreshRect(updateRect, true);
}

void wxPDFViewImpl::OnPaint(wxPaintEvent& event)
{
	wxSize clientSize = m_ctrl->GetClientSize();

	wxAutoBufferedPaintDC dc(m_ctrl);
	m_ctrl->PrepareDC(dc);

	wxSharedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));

	wxRect rectUpdate = m_ctrl->GetUpdateClientRect();
	rectUpdate.SetPosition(m_ctrl->CalcUnscrolledPosition(rectUpdate.GetPosition()));
	rectUpdate = ScaledToUnscaled(rectUpdate);

	dc.SetBackground(m_ctrl->GetBackgroundColour());
	dc.Clear();

	// Draw visible pages
	std::set<int> requiredBitmaps;
	bool pageRendered = false;
	int pageIndex = 0;
	for (auto it = m_pageRects.begin(); it != m_pageRects.end(); ++it, ++pageIndex)
	{
		if (it->Intersects(rectUpdate))
		{
			bool hasBitmap = DrawPage(*gc, pageIndex, *it);
			if (!pageRendered && m_currentPage != pageIndex)
			{
				m_currentPage = pageIndex;
				wxCommandEvent* evt = new wxCommandEvent(wxEVT_PDFVIEW_PAGE_CHANGED);
				evt->SetInt(pageIndex);
				m_ctrl->QueueEvent(evt);
			}
			pageRendered = true;

			if (!hasBitmap)
				requiredBitmaps.insert(pageIndex);
		}
		else if (pageRendered)
			break;
	}

	if (!requiredBitmaps.empty())
	{
		wxCriticalSectionLocker csl(m_bmpRequestCS);
		m_bmpRequest = requiredBitmaps;
		m_bmpRequestHandlerCondition->Broadcast();
	}
}

void wxPDFViewImpl::OnSize(wxSizeEvent& event)
{
	AlignPageRects();
	event.Skip();
}

void wxPDFViewImpl::OnMouseWheel(wxMouseEvent& event)
{
	if (event.ControlDown() && event.GetWheelRotation() != 0)
	{
		int currentZoom = m_zoom;

		int delta;
		if ( currentZoom < 100 )
			delta = 5;
		else if ( currentZoom <= 120 )
			delta = 10;
		else
			delta = 50;

		if ( event.GetWheelRotation() < 0 )
			delta = -delta;

		SetZoom(currentZoom + delta);
	} else
		event.Skip();
}

void wxPDFViewImpl::OnMouseMotion(wxMouseEvent& event)
{
	if (GetLinkTargetPageAtClientPos(event.GetPosition()) >= 0)
		m_ctrl->SetCursor(m_handCursor);
	else
		m_ctrl->SetCursor(wxCURSOR_ARROW);

	event.Skip();
}

void wxPDFViewImpl::OnMouseLeftUp(wxMouseEvent& event)
{
	int linkPage = GetLinkTargetPageAtClientPos(event.GetPosition());
	if (linkPage >= 0)
		SetCurrentPage(linkPage);

	event.Skip();
}

void wxPDFViewImpl::OnScroll(wxScrollWinEvent& event)
{
	// TODO: figure out how to get scroll position in this event on MSW

	event.Skip();
}

void wxPDFViewImpl::SetCurrentPage(int pageIndex)
{
	if (pageIndex < 0)
		pageIndex = 0;
	else if (pageIndex >= m_pageCount)
		pageIndex = m_pageCount -1;

	wxRect pageRect = UnscaledToScaled(m_pageRects[pageIndex]);
	m_ctrl->Scroll(0, pageRect.GetTop() / m_scrollStepY);
}

int wxPDFViewImpl::GetCurrentPage() const
{
	return m_currentPage;
}

void wxPDFViewImpl::UpdateVirtualSize()
{
	int virtualHeight = m_docSize.y + (m_pageCount * m_pagePadding);
	m_ctrl->SetVirtualSize(m_docSize.x * m_ctrl->GetScaleX(), virtualHeight * m_ctrl->GetScaleY());
	m_ctrl->SetScrollRate(m_scrollStepX, m_scrollStepY);
}

void wxPDFViewImpl::SetZoom(int zoom)
{
	if (zoom < m_minZoom)
		zoom = m_minZoom;
	else if (zoom > m_maxZoom)
		zoom = m_maxZoom;

	m_zoom = zoom;

	m_ctrl->SetScale(m_zoom / (double) 100, m_zoom / (double) 100);

	UpdateVirtualSize();
	AlignPageRects();
	m_ctrl->Refresh();
	m_ctrl->ProcessEvent(wxCommandEvent(wxEVT_PDFVIEW_ZOOM_CHANGED));
}

bool wxPDFViewImpl::LoadStream(wxSharedPtr<std::istream> pStream)
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

	FPDF_BYTESTRING pdfPassword = NULL;
	m_pdfDoc = FPDFAvail_GetDocument(m_pdfAvail, pdfPassword);

	if (!m_pdfDoc)
	{
		LogPDFError();
		return false;
	}

	unsigned long docPermissions = FPDF_GetDocPermissions(m_pdfDoc);
	// TODO: check permissions
	(void) FPDFAvail_IsFormAvail(m_pdfAvail, &hints);

	m_pdfForm = FPDFDOC_InitFormFillEnviroument(m_pdfDoc, &form_callbacks);
	FPDF_SetFormFieldHighlightColor(m_pdfForm, 0, 0xFFE4DD);
	FPDF_SetFormFieldHighlightAlpha(m_pdfForm, 100);

	int first_page = FPDFAvail_GetFirstPageNum(m_pdfDoc);
	(void) FPDFAvail_IsPageAvail(m_pdfAvail, first_page, &hints);

	m_pageCount = FPDF_GetPageCount(m_pdfDoc);
	m_pageRects.reserve(m_pageCount);

	m_docSize.Set(0, 0);
	wxRect pageRect;
	pageRect.y += m_pagePadding / 2;
	for (int i = 0; i < m_pageCount; ++i)
	{
		(void) FPDFAvail_IsPageAvail(m_pdfAvail, i, &hints);

		double width;
		double height;
		if (FPDF_GetPageSizeByIndex(m_pdfDoc, i, &width, &height))
		{
			pageRect.width = width;
			pageRect.height = height;
			m_pageRects.push_back(pageRect);

			if (width > m_docSize.x)
				m_docSize.x = width;

			pageRect.y += height;
			pageRect.y += m_pagePadding;
		} else {
			// Document broken?
			m_pageCount = i;
			break;
		}
	}
	m_docSize.SetHeight(pageRect.y);

	AlignPageRects();

	m_ctrl->Refresh();
	UpdateDocumentInfo();

	FORM_DoDocumentJSAction(m_pdfForm);
	FORM_DoDocumentOpenAction(m_pdfForm);

	return true;
}

void wxPDFViewImpl::CloseDocument()
{
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
	m_bitmapCache.Clear();
	m_pageRects.clear();
	m_pageCount = 0;
	m_docSize.Set(0, 0);
	UpdateVirtualSize();
}

wxThread::ExitCode wxPDFViewImpl::Entry()
{
	wxMutex requestHandlerMutex;
	requestHandlerMutex.Lock();
	m_bmpRequestHandlerCondition = new wxCondition(requestHandlerMutex);

	while (m_bmpRequestHandlerActive)
	{
		m_bmpRequestHandlerCondition->Wait();

		while (!m_bmpRequest.empty() && m_bmpRequestHandlerActive)
		{
			int requestedPageIndex;

			{
				wxCriticalSectionLocker csl(m_bmpRequestCS);
				requestedPageIndex = *m_bmpRequest.begin();
			}

			RenderPage(requestedPageIndex);

			{
				wxCriticalSectionLocker csl(m_bmpRequestCS);
				m_bmpRequest.erase(requestedPageIndex);
			}
		}
	}

	return 0;
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
	wxPoint docPos = m_ctrl->CalcUnscrolledPosition(clientPos);
	docPos.x /= m_ctrl->GetScaleX();
	docPos.y /= m_ctrl->GetScaleY();

	for (int i = 0; i < m_pageCount; ++i)
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

int wxPDFViewImpl::GetLinkTargetPageAtClientPos(const wxPoint& clientPos)
{
	int targetPageIndex = -1;

	wxPoint pagePos;
	int pageIndex = ClientToPage(clientPos, pagePos);
	if (pageIndex >= 0)
	{
		FPDF_PAGE page = FPDF_LoadPage(m_pdfDoc, pageIndex);
		// Mouse movement on page check
		wxRect pageRect = m_pageRects[pageIndex];
		double page_x;
		double page_y;
		FPDF_DeviceToPage(page, 0, 0, pageRect.width, pageRect.height, 0, pagePos.x, pagePos.y, &page_x, &page_y);
		FPDF_LINK link = FPDFLink_GetLinkAtPoint(page, page_x, page_y);
		if (link)
		{
			FPDF_DEST dest = FPDFLink_GetDest(m_pdfDoc, link);
			if (!dest)
			{
				FPDF_ACTION action = FPDFLink_GetAction(link);
				if (action)
					dest = FPDFAction_GetDest(m_pdfDoc, action);
			}
			
			if (dest)
				targetPageIndex = FPDFDest_GetPageIndex(m_pdfDoc, dest);
		}
		FPDF_ClosePage(page);
	}

	return targetPageIndex;
}
