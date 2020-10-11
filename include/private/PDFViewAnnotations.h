/////////////////////////////////////////////////////////////////////////////
// Name:        include/private/PDFView.h
// Purpose:     wxPDFViewAnnotations class
// Author:      Markus Pingel
// Created:     2020-09-24
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PDFVIEW_ANNOTATIONS_H
#define PDFVIEW_ANNOTATIONS_H

#include "PDFView.h"
#include "fpdf_doc.h"
#include "fpdf_annot.h"
#include "PDFViewPages.h"
#include "PDFViewTextRange.h"

class wxPDFViewAnnotations
{
public:
	wxPDFViewAnnotations(wxPDFViewPages pages);

	wxPDFViewAnnotation* GetAnnotationAtPos(FPDF_PAGE page, float x, float y);

	std::string GetJsonSerialized();

	wxPDFViewAnnotation* CreateAnnotation(FPDF_PAGE page, FPDF_ANNOTATION_SUBTYPE subtype, FS_RECTF* annotRect);

	wxPDFViewAnnotation* CreateAnnotation(FPDF_PAGE page, FPDF_ANNOTATION_SUBTYPE subtype, FS_RECTF* annotRect, wxPDFViewTextRange* textRange);
private:
	wxVector<wxPDFViewAnnotation*> m_annotations;
};

#endif // PDFVIEW_ANNOTATIONS_H
