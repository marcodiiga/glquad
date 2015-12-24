#ifndef HEADER_IMAGEUTILS_HPP
#define HEADER_IMAGEUTILS_HPP

#include <png.h>
#include <pngstruct.h>
#include <pnginfo.h>
#include <string>
#include <vector>
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
    fprintf(stderr, "error: %s is not a PNG.\n", file_name);
    fclose(fp);
    return 0;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
  {
    fprintf(stderr, "error: png_create_read_struct returned 0.\n");
    fclose(fp);
    return 0;
  }

  // create png info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return 0;
  }

  // create png info struct
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    return 0;
  }

  // the code in this if statement gets called if libpng encounters an error
  if (setjmp(png_jmpbuf(png_ptr))) {
    fprintf(stderr, "error from libpng\n");
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

  if (width) { width = temp_width; }
  if (height) { height = temp_height; }

  //printf("%s: %lux%lu %d\n", file_name, temp_width, temp_height, color_type);

  if (bit_depth != 8)
  {
    fprintf(stderr, "%s: Unsupported bit depth %d.  Must be 8.\n", file_name, bit_depth);
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
    fprintf(stderr, "%s: Unknown libpng color type %d.\n", file_name, color_type);
    return 0;
  }

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // glTexImage2d requires rows to be 4-byte aligned
  rowbytes += 3 - ((rowbytes - 1) % 4);

  // Allocate the image_data as a big block, to be given to opengl
  image_data.resize(rowbytes * temp_height * sizeof(png_byte) + 15);  

  // row_pointers is for pointing to image_data for reading the png with libpng
  std::unique_ptr<png_byte*> row_pointers(new png_byte*[temp_height * sizeof(png_byte *)]);

  // set the individual row_pointers to point at the correct offsets of image_data
  for (unsigned int i = 0; i < temp_height; i++) 
  {
    (row_pointers.get())[temp_height - 1 - i] = image_data.data() + i * rowbytes;
  }

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers.get());

  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);
  return true;
}



#endif // HEADER_IMAGEUTILS_HPP
