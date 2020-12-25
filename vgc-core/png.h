#pragma once

#include "pch.h"
#include "image-data.h"

namespace vgc
{
	/*
     * Save an image using the Portable Network Graphics format
     * to a given file path.
     */
	HRESULT SaveImageAsPngFileW(ImageData& img, LPCWSTR path);

	/*
	 * Load an image using the Portable Network Graphics format
	 * from a given file path, and store the result into the given
	 * ImageData object. Should only be used with images saved
	 * using SaveImageAsPngFileW.
	 */
	HRESULT LoadImageFromPngFileW(ImageData& img, LPCWSTR path);
}
