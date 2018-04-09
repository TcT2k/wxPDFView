## Name:        CMakeLists.txt
## Purpose:     Sample appliction CMake file
## Author:      Tobias Taschner
## Created:     2014-08-07
## Copyright:   (c) 2014 Tobias Taschner
## Licence:     wxWindows licence

cmake_minimum_required(VERSION 2.8)

project(wxPDFViewSimpleSample)

find_package(wxWidgets COMPONENTS core base adv REQUIRED)

include( ${wxWidgets_USE_FILE} )

add_subdirectory(../.. wxPDFView)

include_directories(${wxPDFView_INCLUDE_DIRS})

if(APPLE)
	set(SRCS ${SRCS} wxmac.icns)
	SET_SOURCE_FILES_PROPERTIES(
		wxmac.icns
		PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources
	)
  set(MACOSX_BUNDLE_ICON_FILE "wxmac.icns")
endif()

add_executable(wxPDFViewSimpleSample WIN32 MACOSX_BUNDLE Simple.cpp Simple.h ${SRCS})
target_link_libraries(wxPDFViewSimpleSample ${wxWidgets_LIBRARIES} wxPDFView)
if(APPLE)
	# Force libc++ (required for pdfium)
	if(CMAKE_GENERATOR STREQUAL "Xcode")
		set_target_properties(wxPDFViewSimpleSample PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY libc++)
	else()
		target_compile_options(wxPDFViewSimpleSample PUBLIC "-stdlib=libc++")
		target_link_libraries(wxPDFViewSimpleSample "-stdlib=libc++")
	endif()
endif()
