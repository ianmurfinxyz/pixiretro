
//
// TODO - There is a fair amount of diplicate code in this class. Could be improved.
//

class HUD
{
public:
  using uid_t = int32_t;

  struct TextLabel
  {
    TextLabel(Vector2i p, Color3f c, std::string t, float activeDelay = 0.f, bool phase = false, bool flash = false);

    TextLabel(const TextLabel&) = default;
    TextLabel(TextLabel&&) = default;
    TextLabel& operator=(const TextLabel&) = default;
    TextLabel& operator=(TextLabel&&) = default;

    uid_t _uid;
    Vector2i _position;
    Color3f _color;
    std::string _text;       // Source text.
    std::string _value;      // The text being shown.
    int32_t _charNo;         // The index into '_text' of the last character shown if '_phaseIn' = true.
    float _activeDelay;
    float _activeTime;
    bool _isActive;          // Allows delayed activation.
    bool _isVisible;         // Used internally to flash the label.
    bool _isHidden;          // Allows manual hiding.
    bool _phase;
    bool _flash;
  };

  struct IntLabel
  {
    IntLabel(Vector2i p, Color3f c, const int32_t* s, int32_t precision, float activeDelay = 0.f, bool flash = false);

    IntLabel(const IntLabel&) = default;
    IntLabel(IntLabel&&) = default;
    IntLabel& operator=(const IntLabel&) = default;
    IntLabel& operator=(IntLabel&&) = default;

    uid_t _uid;
    Vector2i _position;
    Color3f _color;
    const int32_t* _source;  // The integer whom's value to display.
    int32_t _value;          // The current value of the source.
    int32_t _precision;      // Number of digits to display.
    std::string _text;       // Text generated from the value.
    float _activeDelay;
    float _activeTime;
    bool _isActive;
    bool _isVisible;
    bool _isHidden;
    bool _flash;
  };

  struct BitmapLabel
  {
    BitmapLabel(Vector2i p, Color3f c, const Bitmap* b, float activeDelay = 0.f, bool flash = false);

    BitmapLabel(const BitmapLabel&) = default;
    BitmapLabel(BitmapLabel&&) = default;
    BitmapLabel& operator=(const BitmapLabel&) = default;
    BitmapLabel& operator=(BitmapLabel&&) = default;

    uid_t _uid;
    Vector2i _position;
    Color3f _color;
    const Bitmap* _bitmap;
    float _activeDelay;
    float _activeTime;
    bool _isActive;
    bool _isVisible;
    bool _isHidden;
    bool _flash;
  };

public:
  HUD() = default;
  ~HUD() = default;
  HUD(const HUD&) = default;
  HUD(HUD&&) = default;
  HUD& operator=(const HUD&) = default;
  HUD& operator=(HUD&&) = default;

  void initialize(const Font* font, float flashPeriod, float phaseInPeriod);

  uid_t addTextLabel(TextLabel label);
  uid_t addIntLabel(IntLabel label);
  uid_t addBitmapLabel(BitmapLabel label);

  bool removeTextLabel(uid_t uid);
  bool removeIntLabel(uid_t uid);
  bool removeBitmapLabel(uid_t uid);

  void clear();

  void hideTextLabel(uid_t uid);
  void hideIntLabel(uid_t uid);
  void hideBitmapLabel(uid_t uid);
  void unhideTextLabel(uid_t uid);
  void unhideIntLabel(uid_t uid);
  void unhideBitmapLabel(uid_t uid);

  void startTextLabelFlash(uid_t uid);
  void startIntLabelFlash(uid_t uid);
  void startBitmapLabelFlash(uid_t uid);
  void stopTextLabelFlash(uid_t uid);
  void stopIntLabelFlash(uid_t uid);
  void stopBitmapLabelFlash(uid_t uid);

  void onReset();
  void onUpdate(float dt);
  void onDraw();

  void setFont(const Font* font) {_font = font;}
  void setFlashPeriod(float period) {_flashPeriod = period;}
  void setPhasePeriod(float period) {_phasePeriod = period;}

private:
  void flashLabels();
  void phaseLabels();
  void updateIntLabels();
  void activateLabels();

private:
  const Font* _font;

  std::vector<TextLabel> _textLabels;
  std::vector<IntLabel> _intLabels;
  std::vector<BitmapLabel> _bitmapLabels;

  int32_t _nextUid;

  float _masterClock;     // Unit: seconds. Master timeline which all timing is relative to.
  float _flashPeriod;     // Unit: seconds. Inverse of the frequency of flashing.
  float _phasePeriod;     // Unit: seconds. Period between each subsequent letter appearing.
  float _flashClock;
  float _phaseClock;
};

// 
// A UI text input box.
//
class TextInput
{
public:
  TextInput(Font* font, std::string label, Color3f cursorColor);

  const char* processInput();
  void draw(float dt);

private:
  Font* _font;
  std::vector<char> _buffer;
  int32_t _bufferSize;
  Color3f cursorColor;
  int32_t _cursorPos;
  float _cursorFlashPeriod;
  float _cursorFlashClock;
  std::string label;
  Vector2i _boxPosition;
  int32_t _boxW;
  int32_t _boxH;
};

