wxPDFView
=========

wxPDFView is a set of wxWidgets controls to display and 
navigate PDF documents using Google Chromes [PDFium library][2].

![Screenshot](https://tct2k.github.io/wxPDFView/images/PDFViewDocumentFrame_thumb.png "wxPDFViewDocumentFrame")

Requirements
------------
* [wxWidgets][1]: 3.0.0  or greater
* [PDFium][2]: any version?

Compiling
---------
Build PDFium per instructions in the projects [wiki][3]. Use the 
included CMake file in your project and link wxPDFView lib (will 
link the required pdfium libs).

Using
-----
Currently includes 3 controls
* wxPDFView Main pdf view
* wxPDFViewBookmarksCtrl tree control displaying bookmarks contained in the PDF
* wxPDFViewThumbnailsListBox listbox control for displaying thumbnails
* wxPDFViewDocumentFrame Frame combining all controls into a PDF viewer window

After initializing an instance of wxPDFView call SetPDFView on wxPDFViewBookmarksCtrl 
and/or wxPDFViewThumbnailsListBox. Call wxPDFView->LoadDocument to load a PDF into the 
view.


[1]: http://www.wxwidgets.org
[2]: https://code.google.com/p/pdfium/
[3]: https://code.google.com/p/pdfium/wiki/Build
