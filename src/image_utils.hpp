#ifndef HEADER_IMAGEUTILS_HPP
#define HEADER_IMAGEUTILS_HPP

#include <png.h>
#include <pngstruct.h>
#include <pnginfo.h>
#include <string>
#include <vector>
#include <stdio.h>

bool loadPNGImageData(std::string filename, int& width, int& height, bool& hasAlpha, std::vector<char>& outData) {

  FILE *file = nullptr;
  fopen_s(&file, filename.c_str(), "rb"); // C file facility is required by libpng
  if (file == nullptr)
    return false;

  png_structp png_handle = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_handle == nullptr) {
    fclose(file);
    return false;
  }

  png_infop png_info_handle = png_create_info_struct (png_handle);
  if (png_info_handle == NULL) {
    fclose(file);
    png_destroy_read_struct (&png_handle, NULL, NULL);
    return false;
  }

  if (setjmp(png_jmpbuf(png_handle))) {
    png_destroy_read_struct(&png_handle, &png_info_handle, NULL);
    fclose(file);
    return false;
  }

  png_init_io(png_handle, file);
  unsigned int sigRead = 0;
  png_set_sig_bytes(png_handle, sigRead);

  png_read_png(png_handle, png_info_handle, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);

  width = png_info_handle->width;
  height = png_info_handle->height;

  switch (png_info_handle->color_type) {
    case PNG_COLOR_TYPE_RGBA:
      hasAlpha = true;
      break;
    case PNG_COLOR_TYPE_RGB:
      hasAlpha = false;
      break;
    default:
      // Colortype not supported
      png_destroy_read_struct(&png_handle, &png_info_handle, NULL);
      fclose(file);
      return false;
  }

  size_t rowBytes = png_get_rowbytes(png_handle, png_info_handle);
  outData.resize(rowBytes * height);
  png_bytepp rowPointers = png_get_rows(png_handle, png_info_handle);

  for (int i = 0; i < height; i++)
    memcpy(outData.data() + (rowBytes * (height - 1 - i)), rowPointers[i], rowBytes);

  png_destroy_read_struct(&png_handle, &png_info_handle, NULL);
  fclose(file);

  return true;
}

#endif // HEADER_IMAGEUTILS_HPP