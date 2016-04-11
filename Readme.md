wxPDFView
=========

wxPDFView is a set of wxWidgets controls to display and 
navigate PDF documents using Google Chromes [PDFium library][2].

![Screenshot](https://tct2k.github.io/wxPDFView/images/PDFViewDocumentFrame.png "wxPDFViewDocumentFrame")

Resources
---------
* [Documentation](http://wxpdfview.tc84.net/)
* [Download](https://github.com/TcT2k/wxPDFView/releases)
* [GitHub](https://github.com/TcT2k/wxPDFView)

Features
--------
Currently implemented:
* PDF page display control
* PDF bookmarks control
* PDF thumbnail control
* Zooming
* Search
* Printing
* Forms
* Loading PDF from custom sources (any std::istream)

Currently not implemented:
* Text selection

Requirements
------------
* [wxWidgets][1]: 3.0.0  or newer
* [PDFium][2]: 2705 (b8180d491e0e38544acbe1b59f949ca4a2701374) or newer

Compiling
---------
* Build PDFium per instructions on the [PDFium page][2].
* Use GYP_DEFINES='pdf_enable_xfa=0' when building pdfium
* Use [CMake][4] to build the samples/simple project
* Or use the  included CMake file in your project and link wxPDFView lib (will link the required pdfium libs)

Using
-----
Currently includes 3 classes you can include into your own frames
* wxPDFView Main pdf view
* wxPDFViewBookmarksCtrl tree control displaying bookmarks contained in the PDF
* wxPDFViewThumbnailListBox listbox control for displaying thumbnails

Or you can base your PDF viewing on a complete PDF viewer frame
* wxPDFViewDocumentFrame Combining all controls into a PDF viewer window

After initializing an instance of wxPDFView call wxPDFViewBookmarksCtrl::SetPDFView 
and/or wxPDFViewThumbnailListBox. Call wxPDFView::LoadFile to load a PDF into the 
view.


[1]: http://www.wxwidgets.org
[2]: https://pdfium.googlesource.com/pdfium/
[4]: http://cmake.org/
