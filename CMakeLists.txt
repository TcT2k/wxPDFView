## Name:        CMakeLists.txt
## Purpose:     Library CMake file
## Author:      Tobias Taschner
## Created:     2014-08-07
## Copyright:   (c) 2014 Tobias Taschner
## Licence:     wxWindows licence

cmake_minimum_required(VERSION 2.8)

project(wxPDFView)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

set(WITH_PDFIUM_SOURCE true)

# Required components
find_package(PDFium)
find_package(wxWidgets COMPONENTS core base adv REQUIRED)

include( ${wxWidgets_USE_FILE} )

set(INCLUDES
	PDFView.h
	PDFViewArtProvider.h
	PDFViewBookmarksCtrl.h
	PDFViewDocumentFrame.h
	PDFViewDocumentPanel.h
	PDFViewThumbnailListBox.h
	)
	
set(PRIVATE_INCLUDES
	PDFViewActivityPanel.h
	PDFViewBookmarks.h
	PDFViewImpl.h
	PDFViewPages.h
	PDFViewPrintout.h
	PDFViewTextRange.h
	PDFViewThumbnailListBoxImpl.h
	)
	
set(SOURCES
	PDFView.cpp
	PDFViewImpl.cpp
	PDFViewActivityPanel.cpp
	PDFViewArtProvider.cpp
	PDFViewBookmarks.cpp
	PDFViewBookmarksCtrl.cpp
	PDFViewDocumentFrame.cpp
	PDFViewDocumentPanel.cpp
	PDFViewPages.cpp
	PDFViewPrintout.cpp
	PDFViewTextRange.cpp
	PDFViewThumbnailListBox.cpp
	PDFViewThumbnailListBoxImpl.cpp
	)

foreach(Inc ${INCLUDES})
	set(SRCS ${SRCS} include/${Inc})
endforeach(Inc)
foreach(Inc ${PRIVATE_INCLUDES})
	set(SRCS ${SRCS} include/private/${Inc})
endforeach(Inc)
foreach(Src ${SOURCES})
	set(SRCS ${SRCS} src/${Src})
endforeach(Src)

source_group("Private Headers" REGULAR_EXPRESSION "include/private/*")

set(wxPDFView_INCLUDE_DIRS ${wxPDFView_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/include/
	)

option(wxPDFVIEW_USE_SYSTEM_ZLIB "Use system ZLIB instead of pdfium bundled" ON)

include_directories(${wxPDFView_INCLUDE_DIRS} ${PDFIUM_INCLUDE_DIRS})
add_library(wxPDFView STATIC ${SRCS})
target_link_libraries(wxPDFView ${wxWidgets_LIBRARIES} ${PDFIUM_LIBRARIES})
if(wxPDFVIEW_USE_SYSTEM_ZLIB AND NOT WIN32)
	# Zlib is required if not linked into pdfium
	find_package(zlib)
	target_link_libraries(wxPDFView ${ZLIB_LIBRARIES})
endif()

set_target_properties(wxPDFView PROPERTIES CXX_STANDARD 11)
if(APPLE)
	# Force libc++ (required for pdfium)
	if(CMAKE_GENERATOR STREQUAL "Xcode")
		set_target_properties(wxPDFView PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY libc++)
	else()
		target_compile_options(wxPDFView PUBLIC "-stdlib=libc++")
		target_link_libraries(wxPDFView "-stdlib=libc++")
	endif()
endif()

# External Variables
set(wxPDFView_INCLUDE_DIRS ${wxPDFView_INCLUDE_DIRS} PARENT_SCOPE)
