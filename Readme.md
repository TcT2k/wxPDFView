
wxPDFView is a set of wxWidgets controls to display and 
navigate PDF documents using Google Chromes [PDFium library][2].

![Screenshot](https://tct2k.github.io/wxPDFView/images/PDFViewDocumentFrame.png "wxPDFViewDocumentFrame")

Resources
---------
* [Documentation](https://tct2k.github.io/wxPDFView/)
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
=======
# wxPDFView

wxPDFView is a set of wxWidgets controls to display and navigate PDF documents using Google Chromes [PDFium library][2].

![Screenshot](https://tct2k.github.io/wxPDFView/images/PDFViewDocumentFrame.png "wxPDFViewDocumentFrame")

## Features

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

## Building on OS X / macOS

### 1. Preparations

* Install latest Xcode
* Install [Homebrew][1].
* Install [Chromium Depot Tools][2] and make sure you've set PATH correctly by running `gclient`.
* Install wxWidgets and Cmake:
```
brew install wxmac cmake
```

### 2. Build PDFium

```
mkdir pdfium
cd pdfium
export PDFIUM_DIR=`pwd`
gclient config --unmanaged --spec "solutions=[{'name':'pdfium','url':'https://pdfium.googlesource.com/pdfium.git@chromium/2953','deps_file':'DEPS','managed':False,'custom_deps':{},'safesync_url':''}]"
gclient sync
cd pdfium
gn gen out/Debug --args="pdf_enable_xfa=false pdf_enable_v8=true"
ninja -C out/Debug pdfium
```

### 3. Build wxPDFView with Sample Application

Make sure `PDFIUM_DIR` is set (see 2. above).

```
git clone https://github.com/rsippl/wxPDFView.git
cd wxPDFView/samples/simple
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DPDFIUM_ROOT_DIR=$PDFIUM_DIR/pdfium
make
open wxPDFViewSimpleSample.app
```

## Using

Currently includes 3 classes you can include into your own frames
* `wxPDFView` Main pdf view
* `wxPDFViewBookmarksCtrl` tree control displaying bookmarks contained in the PDF
* `wxPDFViewThumbnailListBox` listbox control for displaying thumbnails

Or you can base your PDF viewing on a complete PDF viewer frame
* `wxPDFViewDocumentFrame` Combining all controls into a PDF viewer window

After initializing an instance of `wxPDFView` call `wxPDFViewBookmarksCtrl::SetPDFView` and/or `wxPDFViewThumbnailListBox`. Call `wxPDFView::LoadFile` to load a PDF into the view.


[1]: http://brew.sh
[2]: https://www.chromium.org/developers/how-tos/install-depot-tools
