cmake_minimum_required(VERSION 2.8)

project(wxPDFView)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

# Required components
find_package(PDFium)
find_package(wxWidgets COMPONENTS core base adv REQUIRED)

include( ${wxWidgets_USE_FILE} )

set(INCLUDES
	PDFView.h
	PDFViewBitmapCache.h
	PDFViewBookmarksCtrl.h
	PDFViewThumbnailListBox.h	
	)
	
set(SOURCES
	PDFView.cpp
	PDFViewBitmapCache.cpp
	PDFViewBookmarksCtrl.cpp
	PDFViewThumbnailListBox.cpp
	)

foreach(Inc ${INCLUDES})
	set(SRCS ${SRCS} include/${Inc})
endforeach(Inc)
foreach(Src ${SOURCES})
	set(SRCS ${SRCS} src/${Src})
endforeach(Src)

set(wxPDFView_INCLUDE_DIRS ${wxPDFView_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/include/
	)

include_directories(${wxPDFView_INCLUDE_DIRS} ${PDFIUM_INCLUDE_DIRS})
add_library(wxPDFView STATIC ${SRCS})
target_link_libraries(wxPDFView ${wxWidgets_LIBRARIES} ${PDFIUM_LIBRARIES})

# External Variables
set(wxPDFView_INCLUDE_DIRS ${wxPDFView_INCLUDE_DIRS} PARENT_SCOPE)
