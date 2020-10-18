/////////////////////////////////////////////////////////////////////////////
// Name:        src/PDFViewBookmarks.cpp
// Purpose:     wxPDFViewBookmarks implementation
// Author:      Tobias Taschner
// Created:     2014-08-07
// Copyright:   (c) 2014 Tobias Taschner
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "private/PDFViewAnnotations.h"
#include "private/PDFViewImpl.h"

#include "fpdf_text.h"
#include <fstream>
#include <iomanip>

class wxPDFViewAnnotationImpl: public wxPDFViewAnnotation
{
public:
	wxPDFViewAnnotationImpl(FPDF_PAGE page, int pageIndex, int annotationIndex, int charIndex = -1, int charCount = 0) :
		m_owningPage(page),
		m_annotationIndex(annotationIndex),
		m_handle(NULL),
		m_pageIndex(pageIndex),
		m_persistent(false),
		m_startCharIndex(charIndex),
		m_charCount(charCount)
	{
		Init();
	}

	boolean Init()
	{
		OpenHandle();
		FS_RECTF annotRect;
		FPDFAnnot_GetRect(m_handle, &annotRect);
		int flags = FPDFAnnot_GetFlags(m_handle);
		FPDF_ANNOTATION_SUBTYPE subtype = FPDFAnnot_GetSubtype(m_handle);
		int objectCount = FPDFAnnot_GetObjectCount(m_handle);
		CloseHandle();
		return true;
	}


	wxPDFViewAnnotationImpl(FPDF_ANNOTATION annotation)	
	{
		unsigned long length = 0; // FPDFBookmark_GetTitle(bookmark, NULL, 0);
		if (length > 0)
		{
			char * buffer = new char[length];
			length = 0; // FPDFBookmark_GetTitle(bookmark, buffer, length);
			m_title = wxString(buffer, wxCSConv(wxFONTENCODING_UTF16LE), length);
			delete [] buffer;
		}

	}

	virtual wxString GetTitle() const
	{
		return m_title;
	}

	virtual void Navigate(wxPDFView* pdfView)
	{
		FPDF_DOCUMENT doc = pdfView->GetImpl()->GetDocument();
		
		FPDF_DEST dest = NULL;
	
		if (dest)
		{
			unsigned long pageIndex = FPDFDest_GetDestPageIndex(doc, dest);
			pdfView->GoToPage(pageIndex);
		}
	}

	boolean GetPersistent() const
	{
		return m_persistent;
	}

	int GetIndex() const
	{
		return m_annotationIndex;
	}

	FPDF_PAGE GetOwner() const
	{
		return m_owningPage;
	}

	int GetPageIndex() const
	{
		return m_pageIndex;
	}

	int GetCharIndex() const
	{
		return m_startCharIndex;
	}

	int GetCharCount() const
	{
		return m_charCount;
	}
protected:
	FPDF_ANNOTATION OpenHandle()
	{
		if (!m_handle)
		{
			m_handle = FPDFPage_GetAnnot(m_owningPage, m_annotationIndex);
		}
		return m_handle;
	}

	void CloseHandle()
	{
		FPDFPage_CloseAnnot(m_handle);
		m_handle = NULL;
	}

private:
	wxString m_title;
	boolean m_persistent; // true if the annotation is stored in the PDF file
	FPDF_PAGE m_owningPage;
	int m_pageIndex;
	int m_annotationIndex;
	FPDF_ANNOTATION m_handle;
	int m_startCharIndex;
	int m_charCount;
};

wxPDFViewAnnotations::wxPDFViewAnnotations(wxPDFViewPages* pages):
	m_viewPages(pages)
{
	// Get all annotations of the document
	for (wxPDFViewPage& page : *pages)
	{
		FPDF_PAGE pageHandle = page.GetPage();
		int annotation_count = FPDFPage_GetAnnotCount(pageHandle);
		for (int i = 0; i < annotation_count; i++)
		{
			// the handle might change after closing the handle of a specific annotation
			wxPDFViewAnnotation* annotation = new wxPDFViewAnnotationImpl(pageHandle, page.GetIndex(), i);
			m_annotations.push_back(annotation);
		}
		// page.Unload();
	}
}

FPDF_ANNOTATION wxPDFViewAnnotations::GetAnnotationAtPos(FPDF_PAGE page, float x, float y)
{
	// TODO: use m_annotations
	int annotation_count = FPDFPage_GetAnnotCount(page);
	for (int i = 0; i < annotation_count; i++)
	{
		FPDF_ANNOTATION annotHandle = FPDFPage_GetAnnot(page, i);
		FS_RECTF annotRect;
		FPDFAnnot_GetRect(annotHandle, &annotRect);
		if ((annotRect.left < x) && (annotRect.right > x) && (annotRect.top > y) && (annotRect.bottom < y))
		{
			//  TODO: PageIndex
			// wxPDFViewAnnotation* annotation = new wxPDFViewAnnotationImpl(page, 0, i);
			return annotHandle;
		}
	}
	return nullptr;
}

wxPDFViewAnnotation* wxPDFViewAnnotations::CreateAnnotation(FPDF_ANNOTATION_SUBTYPE subtype, FS_RECTF* annotRect, wxPDFViewTextRange* textRange)
{
	// TODO: Create annotation for page; Get Index of annotation on page
	wxPDFViewAnnotationImpl* annotation = nullptr;
	FPDF_PAGE page = textRange->GetPage()->GetPage();
	FPDF_ANNOTATION annotationHandle = FPDFPage_CreateAnnot(page, subtype);
	if (!!annotationHandle)
	{
		
		int annotationIndex = FPDFPage_GetAnnotIndex(page, annotationHandle);
		int pageIndex = textRange->GetPage()->GetIndex();
		annotation = new wxPDFViewAnnotationImpl(page, pageIndex, annotationIndex, textRange->GetCharIndex(), textRange->GetCharCount());
		auto rectSet = FPDFAnnot_SetRect(annotationHandle, annotRect);
		FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
		int count = FPDFText_CountRects(textPage, textRange->GetCharIndex(), textRange->GetCharCount());
		for (int i = 0; i < count; ++i)
		{
			double left, top, right, bottom;
			FPDFText_GetRect(textPage, i, &left, &top, &right, &bottom);
			FS_QUADPOINTSF quadPoints;
			quadPoints.x1 = left;
			quadPoints.y1 = top;
			quadPoints.x2 = right;
			quadPoints.y2 = top;
			quadPoints.x3 = left;
			quadPoints.y3 = bottom;
			quadPoints.x4 = right;
			quadPoints.y4 = bottom;
			FPDFAnnot_AppendAttachmentPoints(annotationHandle, &quadPoints);
		}

		m_annotations.push_back(annotation);
		
	}
	return annotation;
}

int wxPDFViewAnnotations::CreateAnnotationsFromJson()
{
	std::ifstream i("C:\\temp\\annot.json");
	json j;
	i >> j;
	if (j.is_array())
	{
		for (auto& element : j) {
			std::string s = element["select"]["text"];
			int charIndex = element["select"]["start"];
			int charCount = element["select"]["count"];
			int pageIndex = element["page"];
			if (charCount != 0)
			{
				wxPDFViewPage& viewPage = m_viewPages->at(pageIndex);
				wxPDFViewTextRange* textRange = new wxPDFViewTextRange(&viewPage, charIndex, charCount);
				FS_RECTF annotRect;
				annotRect.left = element["rect"].at(0);
				annotRect.bottom = element["rect"].at(1);
				annotRect.right = element["rect"].at(2);
				annotRect.top = element["rect"].at(3);
				CreateAnnotation(FPDF_ANNOT_HIGHLIGHT, &annotRect, textRange);
			}					
		}
	}
	return 0;
}

std::string wxPDFViewAnnotations::GetJsonSerialized()
{
	json j = json::array();
	for (wxPDFViewAnnotation* annot : m_annotations)
	{
		wxPDFViewAnnotationImpl* annotImpl = static_cast<wxPDFViewAnnotationImpl*>(annot);
		if (!annotImpl->GetPersistent())
		{
			json j2;
			
			FPDF_PAGE page = annotImpl->GetOwner();
			int annotIndex = annotImpl->GetIndex();
			j2["id"] = annotIndex;
			j2["page"] = annotImpl->GetPageIndex();
			FPDF_ANNOTATION annotHandle = FPDFPage_GetAnnot(page, annotIndex);
			FS_RECTF annotRect;
			FPDFAnnot_GetRect(annotHandle, &annotRect);
			j2["select"]["start"] = annotImpl->GetCharIndex();
			j2["select"]["count"] = annotImpl->GetCharCount();
			std::wstring text(1024, '\0');
			FPDF_TEXTPAGE textpage = FPDFText_LoadPage(page);
			int textCount = FPDFText_GetText(textpage, annotImpl->GetCharIndex(), annotImpl->GetCharCount(), (unsigned short*)&text[0]);
			text.resize(textCount);		
			wxString wtext = text;
			j2["select"]["text"] = wtext.ToUTF8();
			j2["rect"] = { annotRect.left, annotRect.bottom, annotRect.right, annotRect.top };
			int pointCount = FPDFAnnot_CountAttachmentPoints(annotHandle);
			json points = json::array();
			for (int i = 0; i < pointCount; i++)
			{
				FS_QUADPOINTSF quadPoints;
				FPDFAnnot_GetAttachmentPoints(annotHandle, i, &quadPoints);
				json jp = { quadPoints.x1, quadPoints.x2, quadPoints.x3, quadPoints.x4, quadPoints.y1, quadPoints.y2, quadPoints.y3, quadPoints.y4 };
				points.push_back(jp);
			}
			j2["points"] = points;
			
			
			j.push_back(j2);
		}
	}
	std::ofstream o("C:\\temp\\file.json");
	o << std::setw(4) << j << std::endl;
	return j.dump(2);
}
