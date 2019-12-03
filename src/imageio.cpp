// Written by Maarten ter Huurne in 2011.
// Based on the libpng example.c source, with some additional inspiration
// from SDL_image and openMSX.
// License: GPL version 2 or later.
#include "imageio.h"
#include "debug.h"

#include <SDL.h>
#include <limits.h>
#include <png.h>
#include <cassert>

#ifdef HAVE_LIBOPK
#include <opk.h>

static void __readFromOpk(png_structp png_ptr, png_bytep ptr, png_size_t length)
{
	char **buf = (char **) png_get_io_ptr(png_ptr);

	memcpy(ptr, *buf, length);
	*buf += length;
}

#endif

/* png saving section */
static int png_colortype_from_surface(SDL_Surface *surface) {
	int colortype = PNG_COLOR_MASK_COLOR; /* grayscale not supported */

	if (surface->format->palette)
		colortype |= PNG_COLOR_MASK_PALETTE;
	else if (surface->format->Amask)
		colortype |= PNG_COLOR_MASK_ALPHA;
		
	return colortype;
}

void png_user_warn(png_structp ctx, png_const_charp str) {
	fprintf(stderr, "libpng: warning: %s\n", str);
}

void png_user_error(png_structp ctx, png_const_charp str) {
	fprintf(stderr, "libpng: error: %s\n", str);
}

int saveSurfacePng(char *filename, SDL_Surface *surf) {
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	int colortype;
	png_bytep *row_pointers;

	TRACE("saveSurfacePng - enter : %s", filename);

	TRACE("saveSurfacePng - flipping rgb <-> bgr - w: %i, h: %i, d: %i, p: %i", 
		surf->w, 
		surf->h, 
		surf->format->BitsPerPixel, 
		surf->pitch);

	SDL_Surface *dest = SDL_CreateRGBSurface(
		SDL_SWSURFACE | SDL_SRCALPHA, 
		surf->w, 
		surf->h, 
		surf->format->BitsPerPixel,
		surf->format->Bmask, 
		surf->format->Gmask, 
		surf->format->Rmask, 
		surf->format->Amask
	);
	if (!dest) {
		ERROR("saveSurfacePng - Couldn't create surface");
		return -1;
	}
	TRACE("saveSurfacePng - created destination surface - w: %i, h: %i, d: %i, p: %i", 
		dest->w, 
		dest->h, 
		dest->format->BitsPerPixel, 
		dest->pitch);

	// we need to do this or we get a blank image!!
	SDL_SetAlpha(surf, 0, 0);
	int ret = SDL_BlitSurface(surf, NULL, dest, NULL);
	if (ret != 0) {
		ERROR("saveSurfacePng - Couldn't blit to new surface, result was : %i", ret);
		return -1;
	}

	TRACE("saveSurfacePng - getting colortype");
	colortype = png_colortype_from_surface(dest);
	TRACE("saveSurfacePng - got colortype : %i", colortype);

	/* Opening output file */
	TRACE("saveSurfacePng - opening output file");
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		perror("fopen error");
		return -1;
	}
	TRACE("saveSurfacePng - opened for writing");

	/* Initializing png structures and callbacks */
	png_ptr = png_create_write_struct(
		PNG_LIBPNG_VER_STRING, 
		NULL, 
		png_user_error, 
		png_user_warn);

	if (png_ptr == NULL) {
		printf("png_create_write_struct error!\n");
		return -1;
	}
	TRACE("saveSurfacePng - created write struct");

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		printf("png_create_info_struct error!\n");
		exit(-1);
	}
	TRACE("saveSurfacePng - created info struct");

	// error handler
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		exit(-1);
	}

	TRACE("saveSurfacePng - io intialised");
	png_init_io(png_ptr, fp);

	TRACE("saveSurfacePng - setting IHDR - w: %i, h: %i, colortype: %i", dest->w, dest->h, colortype);
	png_set_IHDR(png_ptr, 
		info_ptr, 
		dest->w, 
		dest->h, 
		8, 
		colortype,	
		PNG_INTERLACE_NONE, 
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT);

	TRACE("saveSurfacePng - doing the write");
	png_write_info(png_ptr, info_ptr);
	png_set_packing(png_ptr);

	TRACE("saveSurfacePng - iterating the row pointers");
	row_pointers = (png_bytep*) malloc(sizeof(png_bytep)*dest->h);
	for (int i = 0; i < dest->h; i++)
		row_pointers[i] = (png_bytep)(Uint8 *)dest->pixels + (i * dest->pitch);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);

	/* Cleaning up... */
	SDL_FreeSurface(dest);
	dest = NULL;
	TRACE("saveSurfacePng - cleaning up");
	free(row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	TRACE("saveSurfacePng - exit");
	return 0;
}
/* png saving section */

SDL_Surface *loadPNG(const std::string &path, bool loadAlpha) {
	// Declare these with function scope and initialize them to NULL,
	// so we can use a single cleanup block at the end of the function.
	SDL_Surface *surface = NULL;
	FILE *fp = NULL;
	png_structp png = NULL;
	png_infop info = NULL;
	std::string::size_type pos;
	struct OPK *opk = NULL;
	void *buffer = NULL, *param;

	TRACE("loadPNG - enter : %s", path.c_str());

	// Create and initialize the top-level libpng struct.
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	TRACE("loadPNG - created read structure");
	if (!png) goto cleanup;
	// Create and initialize the image information struct.
	info = png_create_info_struct(png);
	TRACE("loadPNG - created info structure");
	if (!info) goto cleanup;
	// Setup error handling for errors detected by libpng.
	if (setjmp(png_jmpbuf(png))) {
		// Note: This gets executed when an error occurs.
		if (surface) {
			SDL_FreeSurface(surface);
			surface = NULL;
		}
		goto cleanup;
	}

	TRACE("loadPNG - using libOPK");
	pos = path.find('#');
	if (pos != path.npos) {
		TRACE("loadPNG - We have found a hash, time for opk action");
		int ret;
		std::size_t length;

		TRACE("loadPNG - opening opk");
		opk = opk_open(path.substr(0, pos).c_str());
		if (!opk) {
			ERROR("loadPNG - Unable to open OPK\n");
			goto cleanup;
		}

		TRACE("loadPNG - extracing file");
		ret = opk_extract_file(opk, path.substr(pos + 1).c_str(), &buffer, &length);
		if (ret < 0) {
			ERROR("loadPNG - Unable to extract icon from OPK\n");
			goto cleanup;
		}

		param = buffer;
		TRACE("loadPNG - reading png from opk");
		png_set_read_fn(png, &param, __readFromOpk);

	} else {

		TRACE("loadPNG - opening normal png from file system");
		fp = fopen(path.c_str(), "rb");
		if (!fp) goto cleanup;

		// Set up the input control if you are using standard C streams.
		png_init_io(png, fp);
	}

	// The call to png_read_info() gives us all of the information from the
	// PNG file before the first IDAT (image data chunk).
	png_read_info(png, info);
	png_uint_32 width;
	png_uint_32 height;
	int bitDepth, colorType;
	png_get_IHDR(
		png, info, &width, &height, &bitDepth, &colorType, NULL, NULL, NULL);

	TRACE("loadPNG - got image props - w: %i, h: %i, d: %i, c: %i", width, height, bitDepth, colorType);

	// Select ARGB pixel format:
	// (ARGB is the native pixel format for the JZ47xx frame buffer in 24bpp)
	// - strip 16 bit/color files down to 8 bits/color
	png_set_strip_16(png);
	// - convert 1/2/4 bpp to 8 bpp
	png_set_packing(png);
	// - expand paletted images to RGB
	// - expand grayscale images of less than 8-bit depth to 8-bit depth
	// - expand tRNS chunks to alpha channels
	png_set_expand(png);
	// - convert grayscale to RGB
	png_set_gray_to_rgb(png);
	// - add alpha channel
	png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
	// - convert RGBA to ARGB
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		png_set_swap_alpha(png);
	} else {
		png_set_bgr(png); // BGRA in memory becomes ARGB in register
	}

	// Update the image info to the post-conversion state.
	png_read_update_info(png, info);
	png_get_IHDR(
		png, info, &width, &height, &bitDepth, &colorType, NULL, NULL, NULL);
	assert(bitDepth == 8);
	assert(colorType == PNG_COLOR_TYPE_RGB_ALPHA);

	// Refuse to load outrageously large images.
	if (width > 65536) {
		WARNING("Refusing to load image because it is too wide\n");
		goto cleanup;
	}
	if (height > 2048) {
		WARNING("Refusing to load image because it is too high\n");
		goto cleanup;
	}

	// Allocate [A]RGB surface to hold the image.
	TRACE("loadPNG - creating resize surface");
	surface = SDL_CreateRGBSurface(
		SDL_SWSURFACE | SDL_SRCALPHA, 
		width, 
		height, 
		32,
		0x00FF0000, 
		0x0000FF00, 
		0x000000FF, 
		loadAlpha ? 0xFF000000 : 0x00000000
	);
	if (!surface) {
		// Failed to create surface, probably out of memory.
		goto cleanup;
	}

	// Note: GCC 4.9 doesn't want to jump over 'rowPointers' with goto
	//       if it is in the outer scope.
	{
		// Compute row pointers.
		png_bytep rowPointers[height];
		for (png_uint_32 y = 0; y < height; y++) {
			rowPointers[y] =
				static_cast<png_bytep>(surface->pixels) + y * surface->pitch;
		}

		// Read the entire image in one go.
		png_read_image(png, rowPointers);
	}

cleanup:
	// Clean up.
	png_destroy_read_struct(&png, &info, NULL);
	if (fp) fclose(fp);

	if (buffer)
		free(buffer);
	if (opk)
		opk_close(opk);

	return surface;
}
