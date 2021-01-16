// Bits in the bitmap are accessible via a [row][col] position mapped to screen space like:
//
//          row                               
//           ^
//           |                              y
//         7 | | | |█|█| | | |              ^
//         6 | | |█|█|█|█| | |              |      screen-space
//         5 | |█|█|█|█|█|█| |              |         axes
//         4 |█|█| |█|█| |█|█|       ==>    |
//         3 |█|█|█|█|█|█|█|█|              |
//         2 | | |█| | |█| | |              +----------> x
//         1 | |█| |█|█| |█| |
//         0 |█| |█| | |█| |█|           i.e bit[0][0] is the bottom-left most bit.
//           +-----------------> col
//            0 1 2 3 4 5 6 7
//
// Thus the bitmap can be considered to be a coordinate space in which the origin is the bottom-
// left most pixel of the bitmap.

class Bitmap final
{
  friend Assets;

public:
  Bitmap(const Bitmap&) = default;
  Bitmap(Bitmap&&) = default;
  Bitmap& operator=(const Bitmap&) = default;
  Bitmap& operator=(Bitmap&&) = default;
  ~Bitmap() = default;

  bool getBit(int32_t row, int32_t col) const;
  int32_t getWidth() const {return _width;}
  int32_t getHeight() const {return _height;}
  const std::vector<uint8_t>& getBytes() const {return _bytes;}

  void setBit(int32_t row, int32_t col, bool value, bool regen = true);
  void setRect(int32_t rowMin, int32_t colMin, int32_t rowMax, int32_t colMax, bool value, bool regen = true);
  
  void regenerateBytes();

  bool isEmpty();
  bool isApproxEmpty(int32_t threshold);

  void print(std::ostream& out) const;

private:
  Bitmap() = default;

  void initialize(std::vector<std::string> bits, int32_t scale = 1);

private:
  std::vector<std::vector<bool>> _bits;  // used for bit manipulation ops - indexed [row][col]
  std::vector<uint8_t> _bytes;           // used for rendering
  int32_t _width;
  int32_t _height;
};

struct Glyph // note -- cannot nest in font as it needs to be forward declared.
{
  Bitmap _bitmap;
  int32_t _asciiCode;
  int32_t _offsetX;
  int32_t _offsetY;
  int32_t _advance;
  int32_t _width;
  int32_t _height;
};

class Font
{
  friend class Assets;

public:
  struct Meta
  {
    int32_t _lineSpace;
    int32_t _wordSpace;
    int32_t _glyphSpace;
    int32_t _size;
  };

public:
  Font(const Font&) = default;
  Font(Font&&) = default;
  Font& operator=(const Font&) = default;
  Font& operator=(Font&&) = default;

  const Glyph& getGlyph(char c) const {return _glyphs[static_cast<int32_t>(c - '!')];}
  int32_t getLineSpace() const {return _meta._lineSpace;}
  int32_t getWordSpace() const {return _meta._wordSpace;}
  int32_t getGlyphSpace() const {return _meta._glyphSpace;}
  int32_t getSize() const {return _meta._size;}

  int32_t calculateStringWidth(const std::string& str) const;

private:
  Font() = default;
  void initialize(Meta meta, std::vector<Glyph> glyphs);


private:
  std::vector<Glyph> _glyphs;
  Meta _meta;
};

class Color3f
{
  constexpr static float lo {0.f};
  constexpr static float hi {1.f};

public:
  Color3f() : _r{0.f}, _g{0.f}, _b{0.f}{}

  constexpr Color3f(float r, float g, float b) : 
    _r{std::clamp(r, lo, hi)},
    _g{std::clamp(g, lo, hi)},
    _b{std::clamp(b, lo, hi)}
  {}

  Color3f(const Color3f&) = default;
  Color3f(Color3f&&) = default;
  Color3f& operator=(const Color3f&) = default;
  Color3f& operator=(Color3f&&) = default;

  float getRed() const {return _r;}
  float getGreen() const {return _g;}
  float getBlue() const {return _b;}
  void setRed(float r){_r = std::clamp(r, lo, hi);}
  void setGreen(float g){_g = std::clamp(g, lo, hi);}
  void setBlue(float b){_b = std::clamp(b, lo, hi);}

private:
  float _r;
  float _g;
  float _b;
};

namespace colors
{
constexpr Color3f white {1.f, 1.f, 1.f};
constexpr Color3f black {0.f, 0.f, 0.f};
constexpr Color3f red {1.f, 0.f, 0.f};
constexpr Color3f green {0.f, 1.f, 0.f};
constexpr Color3f blue {0.f, 0.f, 1.f};
constexpr Color3f cyan {0.f, 1.f, 1.f};
constexpr Color3f magenta {1.f, 0.f, 1.f};
constexpr Color3f yellow {1.f, 1.f, 0.f};

// greys - more grays: https://en.wikipedia.org/wiki/Shades_of_gray 

constexpr Color3f gainsboro {0.88f, 0.88f, 0.88f};
constexpr Color3f lightgray {0.844f, 0.844f, 0.844f};
constexpr Color3f silver {0.768f, 0.768f, 0.768f};
constexpr Color3f mediumgray {0.76f, 0.76f, 0.76f};
constexpr Color3f spanishgray {0.608f, 0.608f, 0.608f};
constexpr Color3f gray {0.512f, 0.512f, 0.512f};
constexpr Color3f dimgray {0.42f, 0.42f, 0.42f};
constexpr Color3f davysgray {0.34f, 0.34f, 0.34f};
constexpr Color3f jet {0.208f, 0.208f, 0.208f};
};

class Renderer
{
public:
  struct Config
  {
    std::string _windowTitle;
    int32_t _windowWidth;
    int32_t _windowHeight;
    int32_t _openglVersionMajor;
    int32_t _openglVersionMinor;
    bool _fullscreen;
  };
  
public:
  Renderer(const Config& config);
  Renderer(const Renderer&) = delete;
  Renderer* operator=(const Renderer&) = delete;
  ~Renderer();
  void setViewport(iRect viewport);
  void blitText(Vector2f position, const std::string& text, const Font& font, const Color3f& color);
  void blitBitmap(Vector2f position, const Bitmap& bitmap, const Color3f& color);
  void drawBorderRect(const iRect& rect, const Color3f& background, const Color& borderColor, int32_t borderWidth = 1);
  void clearWindow(const Color3f& color);
  void clearViewport(const Color3f& color);
  void show();
  Vector2i getWindowSize() const;

private:
  SDL_Window* _window;
  SDL_GLContext _glContext;
  Config _config;
  iRect _viewport;
};

extern std::unique_ptr<Renderer> renderer;
