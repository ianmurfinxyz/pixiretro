#ifndef _PIXIRETRO_BMPIMAGE_H_
#define _PIXIRETRO_BMPIMAGE_H_

//----------------------------------------------------------------------------------------------//
// FILE: bmpimage.h                                                                             //
// AUTHOR: Ian Murfin - github.com/ianmurfinxyz                                                 //
//----------------------------------------------------------------------------------------------//

#include <fstream>
#include "color.h"
#include "math.h"

namespace pxr
{
namespace gfx
{

class BmpImage
{
public:
  static constexpr const char* FILE_EXTENSION {".bmp"};

public:
  BmpImage();
  ~BmpImage();

  BmpImage(const BmpImage& other);
  BmpImage(BmpImage&& other);
  BmpImage& operator=(const BmpImage& other);
  BmpImage& operator=(BmpImage&& other);

  bool load(std::string filepath);
  void create(Vector2i size, Color4u fill);

  void clear(Color4u color);

  const Color4u getPixel(int row, int col);
  const Color4u* getRow(int row);
  const Color4u* const* getPixels() const {return _pixels;}

  int getWidth() const {return _size._x;}
  int getHeight() const {return _size._y;}
  Vector2i getSize() const {return _size;}

private:
  static constexpr int BMPMAGIC {0x4D42};
  static constexpr int SRGBMAGIC {0x73524742};

  static constexpr int FILEHEADER_SIZE_BYTES {14};
  static constexpr int V1INFOHEADER_SIZE_BYTES {40};
  static constexpr int V2INFOHEADER_SIZE_BYTES {52};
  static constexpr int V3INFOHEADER_SIZE_BYTES {56};
  static constexpr int V4INFOHEADER_SIZE_BYTES {108};
  static constexpr int V5INFOHEADER_SIZE_BYTES {124};

  // This is a somewhat arbitrary choice made to avoid allocating excessive memory
  // and to aid checking bmp image integrity. It is very game dependent so feel free
  // to adjust these limits to suit your needs.
  static constexpr int BMP_MAX_WIDTH {256};
  static constexpr int BMP_MAX_HEIGHT {128};

  enum Compression
  {
    BI_RGB = 0, 
    BI_RLE8, 
    BI_RLE4, 
    BI_BITFIELDS, 
    BI_JPEG, 
    BI_PNG, 
    BI_ALPHABITFIELDS, 
    BI_CMYK = 11,
    BI_CMYKRLE8, 
    BI_CMYKRLE4
  };

  struct FileHeader
  {
    uint16_t _fileMagic;
    uint32_t _fileSize_bytes;
    uint16_t _reserved0;
    uint16_t _reserved1;
    uint32_t _pixelOffset_bytes;
  };
  
  struct InfoHeader
  {
    uint32_t _headerSize_bytes;
    int32_t  _bmpWidth_px;
    int32_t  _bmpHeight_px;
    uint16_t _numColorPlanes;
    uint16_t _bitsPerPixel; 
    uint32_t _compression; 
    uint32_t _imageSize_bytes;
    int32_t  _xResolution_pxPm;
    int32_t  _yResolution_pxPm;
    uint32_t _numPaletteColors;
    uint32_t _numImportantColors;
    uint32_t _redMask;
    uint32_t _greenMask;
    uint32_t _blueMask;
    uint32_t _alphaMask;
    uint32_t _colorSpaceMagic;
  };

private:
  void freePixels();
  void reallocatePixels();
  void extractIndexedPixels(std::ifstream& file, FileHeader& fileHead, InfoHeader& infoHead);
  void extractPixels(std::ifstream& file, FileHeader& fileHead, InfoHeader& infoHead);

private:
  Color4u** _pixels;    // _pixels = ptr to rows, thus accessed _pixels[row][col]
  Vector2i _size;       // x=width/numCols, y=height/numRows
};

} // namespace gfx
} // namespace pxr

#endif
