# Find module for PDFium

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

set(PDFIUM_ROOT_DIR "" CACHE PATH "PDFium root directory")

set(PDFIUM_BUILD_DIR ${PDFIUM_ROOT_DIR}/build)
		  
set(PDFIUM_FOUND FALSE)

set(PDFIUM_INCLUDE_DIRS
	${PDFIUM_ROOT_DIR}/fpdfsdk/include
	${PDFIUM_ROOT_DIR}/core/include
	${PDFIUM_ROOT_DIR}/v8/include
	)
	
set(PDFIUM_SEARCH_LIBS fdrm
	formfiller
	fpdfapi
	fpdfdoc
	fpdftext
	fxcodec
	fxcrt
	fxedit
	fxge
	icui18n
	icuuc
	javascript
	jsapi
	pdfium
	pdfwindow
	v8_base
	v8_libbase
	v8_libplatform
	v8_snapshot)
	
foreach(LIB ${PDFIUM_SEARCH_LIBS})
	set(PDFIUM_LIBRARIES ${PDFIUM_LIBRARIES}
		debug ${PDFIUM_BUILD_DIR}/Debug/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIB}${CMAKE_STATIC_LIBRARY_SUFFIX}
		optimized ${PDFIUM_BUILD_DIR}/Release/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIB}${CMAKE_STATIC_LIBRARY_SUFFIX})
endforeach()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PDFIUM
	REQUIRED_VARS PDFIUM_ROOT_DIR PDFIUM_INCLUDE_DIRS)
