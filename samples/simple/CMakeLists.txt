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

add_executable(wxPDFViewSimpleSample WIN32 Simple.cpp Simple.h)
target_link_libraries(wxPDFViewSimpleSample ${wxWidgets_LIBRARIES} wxPDFView)
