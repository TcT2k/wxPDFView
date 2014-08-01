#include "PDFView.h"

#include <wx/dcbuffer.h>

#include <v8.h>
#include "fpdfdoc.h"
#include "fpdf_ext.h"
#include "fpdf_dataavail.h"
#include "fpdfformfill.h"
#include "fpdftext.h"
#include "fpdfview.h"

#include <wx/arrimpl.cpp> // This is a magic incantation which must be done! 
WX_DEFINE_OBJARRAY(ArrayOfSize);

wxDECLARE_EVENT(wxEVT_BMP_CACHE_AVAILABLE, wxThreadEvent);

wxDEFINE_EVENT(wxEVT_BMP_CACHE_AVAILABLE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_DOCUMENT_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_PAGE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PDFVIEW_ZOOM_CHANGED, wxCommandEvent);

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

int Get_Block(void* param, unsigned long pos, unsigned char* pBuf,
			  unsigned long size) 
{
	wxPDFView* pdfView = (wxPDFView*) param;
	std::istream* pIstr = pdfView->GetStream();
	pIstr->seekg(pos);
	pIstr->read((char*) pBuf, size);
	if (pIstr->tellg() > 0 && !pIstr->fail())
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
// wxPDFView
//

wxPDFView::wxPDFView(wxWindow *parent,
	wxWindowID winid,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name):
	wxScrolledCanvas(parent, winid, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name),
	m_handCursor(wxCURSOR_HAND)
{
	Init();
}

wxPDFView::~wxPDFView()
{
	// End bitmap request handler
	m_bmpRequestHandlerActive = false;
	m_bmpRequestHandlerCondition->Signal();

	CloseDocument();

	if (m_dataStreamOwned)
		delete m_pDataStream;

	FPDF_DestroyLibrary();
}

void wxPDFView::Init()
{
	m_pagePadding = 16;
	m_scrollStepX = 20;
	m_scrollStepY = 20;
	m_zoom = 100;
	m_minZoom = 10;
	m_maxZoom = 800;
	m_pDataStream = NULL;
	m_dataStreamOwned = false;
	m_pdfDoc = NULL;

	SetBackgroundStyle(wxBG_STYLE_PAINT);

	Bind(wxEVT_PAINT, &wxPDFView::OnPaint, this);
	Bind(wxEVT_MOUSEWHEEL, &wxPDFView::OnMouseWheel, this);
	Bind(wxEVT_MOTION, &wxPDFView::OnMouseMotion, this);
	Bind(wxEVT_LEFT_UP, &wxPDFView::OnMouseLeftUp, this);
	Bind(wxEVT_BMP_CACHE_AVAILABLE, &wxPDFView::OnCacheBmpAvailable, this);
	Bind(wxEVT_SCROLLWIN_LINEUP, &wxPDFView::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_LINEDOWN, &wxPDFView::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_PAGEUP, &wxPDFView::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_PAGEDOWN, &wxPDFView::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_TOP, &wxPDFView::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_BOTTOM, &wxPDFView::OnScroll, this);

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

	UNSUPPORT_INFO unsupported_info;
	memset(&unsupported_info, '\0', sizeof(unsupported_info));
	unsupported_info.version = 1;
	unsupported_info.FSDK_UnSupport_Handler = Unsupported_Handler;

	FSDK_SetUnSpObjProcessHandler(&unsupported_info);
}

void wxPDFView::UpdateDocumentInfo()
{
	// Determine screen size
	m_docHeight = 0;
	m_docMaxWidth = 0;
	for (int i = 0; i < m_pageCount; i++)
	{
		m_docHeight += m_pageSizes[i].GetHeight();
		int pageWidth = m_pageSizes[i].GetWidth();
		if (pageWidth > m_docMaxWidth)
			m_docMaxWidth = pageWidth;
	}

	UpdateVirtualSize();

	ProcessEvent(wxCommandEvent(wxEVT_PDFVIEW_DOCUMENT_READY));
	wxCommandEvent pgEvent(wxEVT_PDFVIEW_PAGE_CHANGED);
	pgEvent.SetInt(0);
	ProcessEvent(pgEvent);
}

wxRect wxPDFView::GetPageRect(int pageIndex) const
{
	wxRect pageRect;
	pageRect.SetSize(m_pageSizes[pageIndex]);
	pageRect.y += m_pagePadding / 2;
	for (int i = 0; i < pageIndex; i++)
	{
		pageRect.y += m_pageSizes[i].GetHeight();
		pageRect.y += m_pagePadding;
	}

	wxSize ctrlSize = GetVirtualSize();
	pageRect.x = ((ctrlSize.GetWidth() / GetScaleX()) - pageRect.GetWidth()) / 2;

	return pageRect;
}

bool wxPDFView::DrawPage(wxGraphicsContext& gc, int pageIndex, const wxRect& pageRect)
{
	// Draw page background
	wxRect bgRect = pageRect.Inflate(2, 2);
	gc.SetBrush(*wxWHITE_BRUSH);
	gc.SetPen(*wxBLACK_PEN);
	gc.DrawRectangle(bgRect.x, bgRect.y, bgRect.width, bgRect.height);

	wxSize bmpSize(m_pageSizes[pageIndex].GetWidth() * GetScaleX(), m_pageSizes[pageIndex].GetHeight() * GetScaleY());
	wxBitmap pageBmp;
	bool matchingBitmap = m_bitmapCache.GetBitmapForPage(pageIndex, bmpSize, pageBmp);
	if (pageBmp.IsOk())
		gc.DrawBitmap(pageBmp, pageRect.x, pageRect.y, pageRect.width, pageRect.height);
	return matchingBitmap;
}

void wxPDFView::RenderPage(int pageIndex)
{
	wxSize bmpSize(m_pageSizes[pageIndex].x * GetScaleX(), m_pageSizes[pageIndex].y * GetScaleY());
	wxLogDebug("Rendering page %d (%dx%d)...", pageIndex, bmpSize.x, bmpSize.y);

	m_bitmapCache.RenderPage(pageIndex, bmpSize, m_pdfDoc, m_pdfForm);

	wxThreadEvent evt(wxEVT_BMP_CACHE_AVAILABLE);
	evt.SetInt(pageIndex);
	AddPendingEvent(evt);
}

void wxPDFView::OnCacheBmpAvailable(wxThreadEvent& event)
{
	wxRect updateRect = GetPageRect(event.GetInt());
	updateRect = UnscaledToScaled(updateRect);
	updateRect.SetPosition(CalcScrolledPosition(updateRect.GetPosition()));

	RefreshRect(updateRect, true);
}

void wxPDFView::OnPaint(wxPaintEvent& event)
{
	wxSize clientSize = GetClientSize();

	wxAutoBufferedPaintDC dc(this);
	PrepareDC(dc);

	wxSharedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));

	wxRect rectUpdate = GetUpdateClientRect();
	rectUpdate.SetPosition(CalcUnscrolledPosition(rectUpdate.GetPosition()));
	rectUpdate = ScaledToUnscaled(rectUpdate);

	dc.SetBackground(wxBrush(*wxLIGHT_GREY));
	dc.Clear();

	// Draw visible pages
	std::set<int> requiredBitmaps;
	bool pageRendered = false;
	for (int i = 0; i < m_pageCount; i++)
	{
		wxRect pageRect = GetPageRect(i);
		if (pageRect.Intersects(rectUpdate))
		{
			bool hasBitmap = DrawPage(*gc, i, pageRect);
			if (!pageRendered && m_currentPage != i)
			{
				m_currentPage = i;
				wxCommandEvent* evt = new wxCommandEvent(wxEVT_PDFVIEW_PAGE_CHANGED);
				evt->SetInt(i);
				QueueEvent(evt);
			}
			pageRendered = true;

			if (!hasBitmap)
				requiredBitmaps.insert(i);
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

void wxPDFView::OnMouseWheel(wxMouseEvent& event)
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

void wxPDFView::OnMouseMotion(wxMouseEvent& event)
{
	if (GetLinkTargetPageAtClientPos(event.GetPosition()) >= 0)
		SetCursor(m_handCursor);
	else
		SetCursor(wxCURSOR_ARROW);

	event.Skip();
}

void wxPDFView::OnMouseLeftUp(wxMouseEvent& event)
{
	int linkPage = GetLinkTargetPageAtClientPos(event.GetPosition());
	if (linkPage >= 0)
		GotoPage(linkPage);

	event.Skip();
}

void wxPDFView::OnScroll(wxScrollWinEvent& event)
{
	// TODO: figure out how to get scroll position in this event on MSW

	event.Skip();
}

void wxPDFView::GotoNextPage()
{
	GotoPage(GetCurrentPage() + 1);
}

void wxPDFView::GotoPrevPage()
{
	GotoPage(GetCurrentPage() + 1);
}

void wxPDFView::GotoPage(int pageIndex)
{
	if (pageIndex < 0)
		pageIndex = 0;
	else if (pageIndex >= m_pageCount)
		pageIndex = m_pageCount -1;

	wxRect pageRect = UnscaledToScaled(GetPageRect(pageIndex));
	Scroll(0, pageRect.GetTop() / m_scrollStepY);
}

int wxPDFView::GetCurrentPage() const
{
	return m_currentPage;
}

void wxPDFView::UpdateVirtualSize()
{
	int virtualHeight = m_docHeight + (m_pageCount * m_pagePadding);
	SetVirtualSize(m_docMaxWidth * GetScaleX(), virtualHeight * GetScaleY());
	SetScrollRate(m_scrollStepX, m_scrollStepY);
}

void wxPDFView::SetZoom(int zoom)
{
	if (zoom < m_minZoom)
		zoom = m_minZoom;
	else if (zoom > m_maxZoom)
		zoom = m_maxZoom;

	m_zoom = zoom;

	SetScale(m_zoom / (double) 100, m_zoom / (double) 100);

	UpdateVirtualSize();
	Refresh();
	ProcessEvent(wxCommandEvent(wxEVT_PDFVIEW_ZOOM_CHANGED));
}

void wxPDFView::ZoomFitToPage()
{

}

void wxPDFView::ZoomFitPageWidth()
{

}

void wxPDFView::LoadFile(const wxString& fileName)
{
	// TODO: create stream and call LoadStream
}

void wxPDFView::LoadStream(std::istream* pStream, bool takeOwnership)
{
	m_pDataStream = pStream;
	m_dataStreamOwned = takeOwnership;

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

	FPDF_FILEACCESS file_access;
	memset(&file_access, '\0', sizeof(file_access));
	file_access.m_FileLen = docFileSize;
	file_access.m_GetBlock = Get_Block;
	file_access.m_Param = this;

	FX_FILEAVAIL file_avail;
	memset(&file_avail, '\0', sizeof(file_avail));
	file_avail.version = 1;
	file_avail.IsDataAvail = Is_Data_Avail;

	FX_DOWNLOADHINTS hints;
	memset(&hints, '\0', sizeof(hints));
	hints.version = 1;
	hints.AddSegment = Add_Segment;

	m_pdfAvail = FPDFAvail_Create(&file_avail, &file_access);

	(void) FPDFAvail_IsDocAvail(m_pdfAvail, &hints);

	FPDF_BYTESTRING pdfPassword = NULL;
	if (!FPDFAvail_IsLinearized(m_pdfAvail)) {
		wxLogDebug("Non-linearized path...");
		m_pdfDoc = FPDF_LoadCustomDocument(&file_access, pdfPassword);
	} else {
		wxLogDebug("Linearized path...");
		m_pdfDoc = FPDFAvail_GetDocument(m_pdfAvail, pdfPassword);
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
	m_pageSizes.Alloc(m_pageCount);
	for (int i = 0; i < m_pageCount; ++i)
	{
		(void) FPDFAvail_IsPageAvail(m_pdfAvail, i, &hints);

		double width;
		double height;
		if (FPDF_GetPageSizeByIndex(m_pdfDoc, i, &width, &height))
		{
			m_pageSizes.Add(wxSize(width, height));
		}
	}

	UpdateDocumentInfo();

	FORM_DoDocumentJSAction(m_pdfForm);
	FORM_DoDocumentOpenAction(m_pdfForm);


}

void wxPDFView::CloseDocument()
{
	if (m_pdfForm)
	{
		FORM_DoDocumentAAction(m_pdfForm, FPDFDOC_AACTION_WC);
		FPDFDOC_ExitFormFillEnviroument(m_pdfForm);
	}
	if (m_pdfDoc)
		FPDF_CloseDocument(m_pdfDoc);
	if (m_pdfAvail)
		FPDFAvail_Destroy(m_pdfAvail);
}

wxThread::ExitCode wxPDFView::Entry()
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

wxRect wxPDFView::UnscaledToScaled(const wxRect& rect) const
{
	wxRect scaledRect;
	scaledRect.x = rect.x * GetScaleX();
	scaledRect.y = rect.y * GetScaleY();
	scaledRect.width = rect.width * GetScaleX();
	scaledRect.height = rect.height * GetScaleX();

	return scaledRect;
}

wxRect wxPDFView::ScaledToUnscaled(const wxRect& rect) const
{
	wxRect scaledRect;
	scaledRect.x = rect.x / GetScaleX();
	scaledRect.y = rect.y / GetScaleY();
	scaledRect.width = rect.width / GetScaleX();
	scaledRect.height = rect.height / GetScaleX();

	return scaledRect;
}

int wxPDFView::ClientToPage(const wxPoint& clientPos, wxPoint& pagePos)
{
	wxPoint docPos = CalcUnscrolledPosition(clientPos);
	docPos.x /= GetScaleX();
	docPos.y /= GetScaleY();

	for (int i = 0; i < m_pageCount; ++i)
	{
		wxRect pageRect = GetPageRect(i);

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

int wxPDFView::GetLinkTargetPageAtClientPos(const wxPoint& clientPos)
{
	int targetPageIndex = -1;

	wxPoint pagePos;
	int pageIndex = ClientToPage(clientPos, pagePos);
	if (pageIndex >= 0)
	{
		FPDF_PAGE page = FPDF_LoadPage(m_pdfDoc, pageIndex);
		// Mouse movement on page check
		wxRect pageRect = GetPageRect(pageIndex);
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
