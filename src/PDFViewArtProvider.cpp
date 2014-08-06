#include "PDFViewArtProvider.h"

#include <wx/mstream.h>

#include "../art/page_fit.h"
#include "../art/page_width.h"

bool wxPDFViewArtProvider::m_registered = false;

wxPDFViewArtProvider::wxPDFViewArtProvider()
{
}

void wxPDFViewArtProvider::Initialize()
{
	if (!m_registered)
	{
		wxArtProvider::PushBack(new wxPDFViewArtProvider);
		m_registered = true;
	}
}

wxBitmap wxPDFViewArtProvider::CreateBitmap(const wxArtID& id,
								  const wxArtClient& client,
								  const wxSize& sizeHint)
{
	#define BITMAP_ARRAY_NAME(name, size) \
		name ## _ ## size ## x ## size ## _png
	#define BITMAP_DATA_FOR_SIZE(name, size) \
		BITMAP_ARRAY_NAME(name, size), sizeof(BITMAP_ARRAY_NAME(name, size))
	#define BITMAP_DATA(name) \
		BITMAP_DATA_FOR_SIZE(name, 16), BITMAP_DATA_FOR_SIZE(name, 24)

	static const struct BitmapEntry
	{
		const char *id;
		const unsigned char *data16;
		size_t len16;
		const unsigned char *data24;
		size_t len24;
	} s_allBitmaps[] =
	{
		{ wxART_PDFVIEW_PAGE_FIT,            BITMAP_DATA(page_fit)          },
		{ wxART_PDFVIEW_PAGE_WIDTH,          BITMAP_DATA(page_width)        },
	};

	#undef BITMAP_ARRAY_NAME
	#undef BITMAP_DATA_FOR_SIZE
	#undef BITMAP_DATA

	for ( unsigned n = 0; n < WXSIZEOF(s_allBitmaps); n++ )
	{
		const BitmapEntry& entry = s_allBitmaps[n];
		if ( entry.id != id )
			continue;

		// This is one of the bitmaps that we have, determine in which size we
		// should return it.

		wxSize size;
		bool sizeIsAHint;
		if ( sizeHint == wxDefaultSize )
		{
			// Use the normal platform-specific icon size.
			size = GetNativeSizeHint(client);

			if ( size == wxDefaultSize )
			{
				// If we failed to get it, determine the best size more or less
				// arbitrarily. This definitely won't look good but then it
				// shouldn't normally happen, all platforms should implement
				// GetNativeSizeHint() properly.
				if ( client == wxART_MENU || client == wxART_BUTTON )
					size = wxSize(16, 16);
				else
					size = wxSize(24, 24);
			}

			// We should return the icon of exactly this size so it's more than
			// just a hint.
			sizeIsAHint = false;
		}
		else // We have a size hint
		{
			// Use it for determining the version of the icon to return.
			size = sizeHint;

			// But we don't need to return the image of exactly the same size
			// as the hint, after all it's just that, a hint.
			sizeIsAHint = true;
		}

		enum
		{
			TangoSize_16,
			TangoSize_24
		} tangoSize;

		// We prefer to downscale the image rather than upscale it if possible
		// so use the smaller one if we can, otherwise the large one.
		if ( size.x <= 16 && size.y <= 16 )
			tangoSize = TangoSize_16;
		else
			tangoSize = TangoSize_24;

		const unsigned char *data;
		size_t len;
		switch ( tangoSize )
		{
			default:
				wxFAIL_MSG( "Unsupported Tango bitmap size" );
				// fall through

			case TangoSize_16:
				data = entry.data16;
				len = entry.len16;
				break;

			case TangoSize_24:
				data = entry.data24;
				len = entry.len24;
				break;
		}

		wxMemoryInputStream is(data, len);

		wxImage image(is, wxBITMAP_TYPE_PNG);
		if ( !image.IsOk() )
		{
			return wxNullBitmap;
		}

		if ( !sizeIsAHint )
		{
			// Notice that this won't do anything if the size is already right.
			image.Rescale(size.x, size.y, wxIMAGE_QUALITY_HIGH);
		}

		return image;
	}

	// Not one of the bitmaps that we support.
	return wxNullBitmap;
}
