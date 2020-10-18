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
	wxPDFViewAnnotations(wxPDFViewPages* pages);

	FPDF_ANNOTATION GetAnnotationAtPos(FPDF_PAGE page, float x, float y);

	std::string GetJsonSerialized();

	wxPDFViewAnnotation* CreateAnnotation(FPDF_ANNOTATION_SUBTYPE subtype, FS_RECTF* annotRect, wxPDFViewTextRange* textRange);
	int CreateAnnotationsFromJson();
private:
	wxPDFViewPages* m_viewPages;
	wxVector<wxPDFViewAnnotation*> m_annotations;
};

#endif // PDFVIEW_ANNOTATIONS_H
