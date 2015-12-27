#ifndef HEADER_IMAGEUTILS_HPP
#define HEADER_IMAGEUTILS_HPP

#include <png.h>
#include <pngstruct.h>
#include <pnginfo.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>

#ifndef _MSC_VER
// fopen_s is microsoft-specific
void fopen_s(FILE **file, const char *filename, const char *mode) {
  *file = fopen(filename, mode);  
}
#endif

bool loadPNGFromFile(const char *file_name, int& width, int& height, GLint& format, std::vector<unsigned char>& image_data)
{
  // Adapted from https://github.com/DavidEGrayson/ahrs-visualizer

  png_byte header[8];

  FILE *fp = nullptr;
  fopen_s(&fp, file_name, "rb");
  if (fp == 0)
  {
    perror(file_name);
    return 0;
  }

  // read the header
  fread(header, 1, 8, fp);

  if (png_sig_cmp(header, 0, 8))
  {
    std::cerr << "Error: " << file_name << " is not a PNG file" << std::endl;
    fclose(fp);
    return 0;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
  {
    std::cerr << "Error: png_create_read_struct returned 0" << std::endl;
    fclose(fp);
    return 0;
  }

  // create png info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    std::cerr << "Error: png_create_info_struct returned 0" << std::endl;
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return 0;
  }

  // create png info struct
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    std::cerr << "Error: png_create_info_struct returned 0" << std::endl;
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    return 0;
  }

  // the code in this if statement gets called if libpng encounters an error
  if (setjmp(png_jmpbuf(png_ptr))) {
    std::cerr << "Error from libpng" << std::endl;
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return 0;
  }

  // init png reading
  png_init_io(png_ptr, fp);

  // let libpng know you already read the first 8 bytes
  png_set_sig_bytes(png_ptr, 8);

  // read all the info up to the image data
  png_read_info(png_ptr, info_ptr);

  // variables to pass to get info
  int bit_depth, color_type;
  png_uint_32 temp_width, temp_height;

  // get info about png
  png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height, &bit_depth, &color_type,
    NULL, NULL, NULL);

  width = temp_width;
  height = temp_height;

  //printf("%s: %lux%lu %d\n", file_name, temp_width, temp_height, color_type);

  if (bit_depth != 8) {
    std::cerr << "Unsupported bit depth " << bit_depth << ". Must be 8." << std::endl;
    return 0;
  }

  switch (color_type)
  {
  case PNG_COLOR_TYPE_RGB:
    format = GL_RGB;
    break;
  case PNG_COLOR_TYPE_RGB_ALPHA:
    format = GL_RGBA;
    break;
  default:
    std::cerr << "Unknown libpng color type " << color_type << std::endl;
    return 0;
  }

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // glTexImage2d requires rows to be 4-byte aligned
  rowbytes += 3 - ((rowbytes - 1) % 4);

  // Allocate the image_data as a big block, to be given to opengl
  image_data.resize(rowbytes * temp_height * sizeof(png_byte));

  // row_pointers is for pointing to image_data for reading the png with libpng
  std::vector<png_byte*> row_pointers(temp_height, nullptr);

  // set the individual row_pointers to point at the correct offsets of image_data
  for (unsigned int i = 0; i < temp_height; i++) 
  {
    row_pointers[temp_height - 1 - i] = static_cast<unsigned char*>(image_data.data()) + i * rowbytes;
  }

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers.data());

  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);
  return true;
}



#endif // HEADER_IMAGEUTILS_HPP
