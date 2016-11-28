# Find module for PDFium

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

set(PDFIUM_ROOT_DIR "" CACHE PATH "PDFium root directory")

if (APPLE)
	set(PDFIUM_BUILD_DIR ${PDFIUM_ROOT_DIR}/out)
	set(PDFIUM_BUILD_SUB_DIR obj/)
else ()
	set(PDFIUM_BUILD_DIR ${PDFIUM_ROOT_DIR}/build)
	set(PDFIUM_BUILD_SUB_DIR lib/)
endif ()
		  
set(PDFIUM_FOUND FALSE)

set(PDFIUM_INCLUDE_DIRS
	${PDFIUM_ROOT_DIR}/fpdfsdk/include
	${PDFIUM_ROOT_DIR}/core/include
	${PDFIUM_ROOT_DIR}/v8/include
	${PDFIUM_ROOT_DIR}/v8
	${PDFIUM_ROOT_DIR}/public
	${PDFIUM_ROOT_DIR}
	)
	
set(PDFIUM_SEARCH_LIBS
	fdrm
	formfiller
	fpdfapi
	fpdfdoc
	fpdftext
	fxcodec
	fxcrt
	fxedit
	fxge
	fxjs
	javascript
	pdfium
	pdfwindow
	)

set(PDFIUM_3RD_PARTY_SEARCH_LIBS
	bigint
	fx_agg
	fx_freetype
	fx_lcms2
	fx_libopenjpeg
	fx_lpng
	fx_zlib
	jpeg
	)

set(PDFIUM_V8_SEARCH_LIBS
	icui18n
	icuuc
	v8_libbase
	v8_libplatform
	v8
	)
	
foreach(LIB ${PDFIUM_SEARCH_LIBS})
	set(PDFIUM_LIBRARIES ${PDFIUM_LIBRARIES}
		debug ${PDFIUM_BUILD_DIR}/Debug/${PDFIUM_BUILD_SUB_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}${LIB}${CMAKE_STATIC_LIBRARY_SUFFIX}
		optimized ${PDFIUM_BUILD_DIR}/Release/${PDFIUM_BUILD_SUB_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}${LIB}${CMAKE_STATIC_LIBRARY_SUFFIX})
endforeach()

foreach(LIB ${PDFIUM_3RD_PARTY_SEARCH_LIBS})
	set(PDFIUM_LIBRARIES ${PDFIUM_LIBRARIES}
		debug ${PDFIUM_BUILD_DIR}/Debug/${PDFIUM_BUILD_SUB_DIR}/third_party/${CMAKE_STATIC_LIBRARY_PREFIX}${LIB}${CMAKE_STATIC_LIBRARY_SUFFIX}
		optimized ${PDFIUM_BUILD_DIR}/Release/${PDFIUM_BUILD_SUB_DIR}/third_party/${CMAKE_STATIC_LIBRARY_PREFIX}${LIB}${CMAKE_STATIC_LIBRARY_SUFFIX})
endforeach()

foreach(LIB ${PDFIUM_V8_SEARCH_LIBS})
	set(PDFIUM_LIBRARIES ${PDFIUM_LIBRARIES}
		debug ${PDFIUM_BUILD_DIR}/Debug/${CMAKE_SHARED_LIBRARY_PREFIX}${LIB}${CMAKE_SHARED_LIBRARY_SUFFIX}
		optimized ${PDFIUM_BUILD_DIR}/Release/${CMAKE_SHARED_LIBRARY_PREFIX}${LIB}${CMAKE_SHARED_LIBRARY_SUFFIX})
endforeach()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PDFIUM
	REQUIRED_VARS PDFIUM_ROOT_DIR PDFIUM_INCLUDE_DIRS)
