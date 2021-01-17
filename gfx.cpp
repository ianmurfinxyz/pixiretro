#include <cstring>

void Bitmap::initialize(std::vector<std::string> bits, int32_t scale)
  // predicate: bit strings must contain only 0's and 1's
{
  // Strip trailing 0's on all rows leaving atleast one 0 on rows consisting of all zeros. 
  // This permits 'padding rows' to be created in bitmaps whilst stripping surplus data.
  for(auto& row : bits){
    while(row.back() == '0' && row.size() != 1){
      row.pop_back();
    }
  }

  // scale bits
  if(scale > 1){
    std::vector<std::string> sbits {};
    std::string srow {};
    for(auto& row : bits){
      for(auto c : row){
        for(int i = 0; i < scale; ++i){
          srow.push_back(c);
        }
      }
      row.shrink_to_fit();
      for(int i = 0; i < scale; ++i){
        sbits.push_back(srow);
      }
      srow.clear();
    }
    bits = std::move(sbits);
  }

  // compute bitmap dimensions
  size_t w {0};
  for(const auto& row : bits){
    w = std::max(w, row.length());
  }
  _width = w;
  _height = bits.size();

  // generate the bit data
  std::vector<bool> brow {};
  brow.reserve(w);
  for(const auto& row : bits){
    brow.clear();
    for(char c : row){
      bool bit = !(c == '0'); 
      brow.push_back(bit); 
    }
    int32_t n = w - row.size();
    while(n-- > 0){
      brow.push_back(false); // pad to make all rows equal length
    }
    brow.shrink_to_fit();
    _bits.push_back(brow);
  }

  regenerateBytes();
}

bool Bitmap::getBit(int32_t row, int32_t col) const
{
  assert(0 <= row && row < _height);
  assert(0 <= col && col < _width);
  return _bits[row][col];
}

void Bitmap::setBit(int32_t row, int32_t col, bool value, bool regen)
{
  assert(0 <= row && row < _height);
  assert(0 <= col && col < _width);
  _bits[row][col] = value;
  if(regen)
    regenerateBytes();
}

void Bitmap::setRect(int32_t rowMin, int32_t colMin, int32_t rowMax, int32_t colMax, bool value, bool regen)
{
  // note - inclusive range of rows and columns, i.e. [rowMin, rowMax] and [colMin, colMax]
  
  assert(rowMin >= 0 && rowMax < _height);
  assert(colMin >= 0 && colMax < _width);

  for(int32_t row = rowMin; row <= rowMax; ++row)
    for(int32_t col = colMin; col <= colMax; ++col)
      _bits[row][col] = value;

  if(regen)
    regenerateBytes();
}

void Font::initialize(Meta meta, std::vector<Glyph> glyphs)
  // predicate: expects glyphs for all ascii codes in the range 33-126 inclusive.
  // predicate: expects glyphs to be in ascending order of ascii code.
{
  _meta = meta;
  _glyphs = std::move(glyphs);
}

void Bitmap::regenerateBytes()
{
  _bytes.clear();
  uint8_t byte {0};
  int32_t bitNo {0};
  for(const auto& row : _bits){
    for(bool bit : row){
      if(bitNo > 7){
        _bytes.push_back(byte);
        byte = 0;
        bitNo = 0;
      }
      if(bit) 
        byte |= 0x01 << (7 - bitNo);
      bitNo++;
    }
    _bytes.push_back(byte);
    byte = 0;
    bitNo = 0;
  }
}

int32_t Font::calculateStringWidth(const std::string& str) const
{
  int32_t sum {0};
  for(char c : str)
    sum += getGlyph(c)._advance + _meta._glyphsSpace;
  return sum;
}

bool Bitmap::isEmpty()
{
  for(const auto& row : _bits)
    for(const auto& bit : row)
      if(bit)
        return false;

  return true;
}

bool Bitmap::isApproxEmpty(int32_t threshold)
{
  int32_t count {0};
  for(const auto& row : _bits){
    for(const auto& bit : row){
      if(bit){
        ++count;
        if(count > threshold)
          return false;
      }
    }
  }
  return true;
}

void Bitmap::print(std::ostream& out) const
{
  for(auto iter = _bits.rbegin(); iter != _bits.rend(); ++iter){
    for(bool bit : *iter){
      out << bit;
    }
    out << '\n';
  }
  out << std::endl;
}

Renderer::Renderer(const Config& config)
{
  _config = config;

  uint32_t flags = SDL_WINDOW_OPENGL;
  if(_config._fullscreen){
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    log->log(Log::INFO, logstr::info_fullscreen_mode);
  }

  std::stringstream ss {};
  ss << "{w:" << _config._windowWidth << ",h:" << _config._windowHeight << "}";
  log->log(Log::INFO, logstr::info_creating_window, std::string{ss.str()});

  _window = SDL_CreateWindow(
      _config._windowTitle.c_str(), 
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      _config._windowWidth,
      _config._windowHeight,
      flags
  );

  if(_window == nullptr){
    log->log(Log::FATAL, logstr::fail_create_window, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  Vector2i wndsize = getWindowSize();
  std::stringstream().swap(ss);
  ss << "{w:" << wndsize._x << ",h:" << wndsize._y << "}";
  log->log(Log::INFO, logstr::info_created_window, std::string{ss.str()});

  _glContext = SDL_GL_CreateContext(_window);
  if(_glContext == nullptr){
    log->log(Log::FATAL, logstr::fail_create_opengl_context, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, _config._openglVersionMajor) < 0){
    nomad::log->log(Log::FATAL, logstr::fail_set_opengl_attribute, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, _config._openglVersionMinor) < 0){
    nomad::log->log(Log::FATAL, logstr::fail_set_opengl_attribute, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  setViewport(iRect{0, 0, _config._windowWidth, _config._windowHeight});
}

Renderer::~Renderer()
{
  SDL_GL_DeleteContext(_glContext);
  SDL_DestroyWindow(_window);
}

void Renderer::setViewport(iRect viewport)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, viewport._w, 0.0, viewport._h, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(viewport._x, viewport._y, viewport._w, viewport._h);
  _viewport = viewport;
}

void Renderer::blitText(Vector2f position, const std::string& text,  const Font& font, const Color3f& color)
{
  glColor3f(color.getRed(), color.getGreen(), color.getBlue());  
  glRasterPos2f(position._x, position._y);

  for(char c : text){
    if(!(' ' <= c && c <= '~'))
      continue;

    if(c == ' '){
      glBitmap(0, 0, 0, 0, font.getWordSpace(), 0, nullptr);
    }
    else{
      const Glyph& g = font.getGlyph(c);      
      glBitmap(g._width, g._height, g._offsetX, g._offsetY, g._advance + font.getGlyphSpace(), 0, g._bitmap.getBytes().data());
    }
  }
}

void Renderer::blitBitmap(Vector2f position, const Bitmap& bitmap, const Color3f& color)
{
  // For viewports which are a subregion of the window the glBitmap function will overdraw
  // the viewport bounds if the bitmap position is within the viewport but the bitmap itself
  // partially falls outside the viewport. Hence will clip any overdraw manually. Note that 
  // this may result in 'underdraw' rather than overdraw.
  if(position._x + bitmap.getWidth() > _viewport._w)
    return;

  if(position._y + bitmap.getHeight() > _viewport._h)
    return;

  glColor3f(color.getRed(), color.getGreen(), color.getBlue());  
  glRasterPos2f(position._x, position._y);
  glBitmap(bitmap.getWidth(), bitmap.getHeight(), 0, 0, 0, 0, bitmap.getBytes().data());
}

void Renderer::drawBorderRect(const iRect& rect, const Color3f& background, const Color3f& borderColor, int32_t borderWidth)
{
  int32_t x1, y1, x2, y2;
  x1 = rect._x - borderThickness;
  y1 = rect._y - borderThickness;
  x2 = rect._x + rect._w + borderThickness;
  y2 = rect._y + rect._h + borderThickness;
  glColor3f(borderColor.getRed(), borderColor.getGreen(), borderColor.getBlue());  
  glRect(x1, y1, x2, y2);
  x1 += borderThickness;
  y1 += borderThickness;
  x2 -= borderThickness
  y2 -= borderThickness
  glColor3f(background.getRed(), background.getGreen(), background.getBlue());  
  glRect(x1, y1, x2, y2);
}

void Renderer::clearWindow(const Color3f& color)
{
  glClearColor(color.getRed(), color.getGreen(), color.getBlue(), 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::clearViewport(const Color3f& color)
{
  glEnable(GL_SCISSOR_TEST);
  glScissor(_viewport._x, _viewport._y, _viewport._w, _viewport._h);
  glClearColor(color.getRed(), color.getGreen(), color.getBlue(), 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
}

void Renderer::show()
{
  SDL_GL_SwapWindow(_window);
}

Vector2i Renderer::getWindowSize() const
{
  Vector2i size;
  SDL_GL_GetDrawableSize(_window, &size._x, &size._y);
  return size;
}

std::unique_ptr<Renderer> renderer {nullptr};


Screen::Screen(Vector2i windowSize, Vector2i screenSize) :
  _pixels{},
  _bands{},
  _position{},
  _size{screenSize},
  _pmode{PositionMode::CENTER},
  _smode{ScaleMode::AUTO_MAX},
  _cmode{ColorMode::FULL_COLOR},
  _pixelSize{}
{
  _pixels.resize(screenSize._x * screenSize._y);
  onWindowResize(windowSize);
}

void Screen::clearColor(const Color4& color)
{
  for(auto& pixel : _pixels)
    pixel._color = color;
}

void Screen::clearShade(int value)
{
  value = std::min(std::max(0, value), 255); 
  memset(_pixels.data(), value, _pixels.size() * sizeof(Pixel));
} 

void Screen::clearTransparent()
{
  memset(_pixels.data(), 255, _pixels.size() * sizeof(Pixel));
} 

void Screen::drawPixel(int row, int col, const Color4& color)
{
  assert(0 <= row && row < _screenSize._y);
  assert(0 <= col && col < _screenSize._x);
  _pixels[col + (row * screenWidth)]._color = color;
}

void Screen::drawSprite(int x, int y, const Sprite& sprite)
{
  assert(x >= 0 && y >= 0);

  const std::vector<Color4>& spritePixels {sprite.getPixels()};
  int spriteWidth {sprite.getWidth()};
  int spriteHeight {sprite.getHeight()};
  
  int spritePixelNum {0};
  for(int spriteRow = 0; spriteRow < spriteHeight; ++spriteRow){
    // if next row is above the screen.
    if(y + spriteRow >= screenHeight)
      break;

    // index of 1st screen pixel in next row being drawn.
    int screenRowIndex {x + ((y + spriteRow) * screenWidth)};   

    for(int spriteCol = 0; spriteCol < spriteWidth; ++spriteCol){
      // if the right side of the sprite falls outside the screen.
      if(x + spriteCol >= screenWidth)
        break;

      _pixels[screenRowIndex + spriteCol]._color = spritePixels[spritePixelNum];  
      ++spritePixelNum;
    }
  }
}

void Screen::onWindowResize(Vector2i windowSize)
{
  // recalculate pixel size.
  if(smode == ScaleMode::AUTO_MAX){
    // note: integer math here thus all division results are implicitly floored as required.
    int pxw = windowSize._x / _screenSize._x;  
    int pxh = windowSize._y / _screenSize._y;
    _pixelSize = std::max(std::min(pxw, pxh), 1);
  }

  // recalculate screen position.
  switch(_pmode)
  {
  case PositionMode::MANUAL:
    // no change.
    break;
  case PositionMode::CENTER:
    _position._x = std::clamp((windowSize._x - (_pixelSize * _screenSize._x)) / 2, 0, windowSize._x);
    _position._y = std::clamp((windowSize._y - (_pixelSize * _screenSize._y)) / 2, 0, windowSize._y);
    break;
  case PositionMode::TOP_LEFT:
    _position._x = 0;
    _position._y = windowSize._y - (_pixelSize * _screenSize._y);
    break;
  case PositionMode::TOP_RIGHT:
    _position._x = windowSize._x - (_pixelSize * _screenSize._x);
    _position._y = windowSize._y - (_pixelSize * _screenSize._y);
    break;
  case PositionMode::BOTTOM_LEFT:
    _position._x = 0;
    _position._y = 0;
    break;
  case PositionMode::BOTTOM_RIGHT:
    _position._x = windowSize._x - (_pixelSize * _screenSize._x);
    _position._y = 0;
    break;
  }

  // recalculate pixel positions.
  
  // Pixels are drawn as an array of points of _pixelSize diameter. When drawing points in opengl, 
  // the position of the point is taken as the center position. For odd pixel sizes e.g. 7 the 
  // center pixel is simply 3,3 (= floor(7/2)). For even pixel sizes e.g. 8 the center of the 
  // pixel is considered the bottom-left pixel in the top-right quadrant, i.e. 4,4 (= floor(8/2)).
  int pixelCenterOffset = _pixelSize / 2;

  for(int row = 0; row < _screenSize._y; ++row){
    for(int col = 0; col < _screenSize._x; ++col){
      Pixel& pixel = _pixels[col + (row * _screenSize._x)];
      pixel._x = _position._x + (col * _pixelSize) + pixelCenterOffset;
      pixel._y = _position._y + (row * _pixelSize) + pixelCenterOffset;
    }
  }
}

void Screen::render()
{
  auto now0 = std::chrono::high_resolution_clock::now();
  sk::renderer->drawPixelArray(0, pixelCount, static_cast<void*>(_pixels.data()), _pixelSize);
  auto now1 = std::chrono::high_resolution_clock::now();
  std::cout << "Screen::render execution time (us): "
            << std::chrono::duration_cast<std::chrono::microseconds>(now1 - now0).count()
            << std::endl;
}

