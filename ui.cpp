
HUD::TextLabel::TextLabel(Vector2i p, Color3f c, std::string t, float activeDelay, bool phase, bool flash) :
  _position{p},
  _color{c},
  _text{t},
  _value{},
  _charNo{0},
  _activeDelay{activeDelay},
  _activeTime{0},
  _isActive{false},
  _isVisible{true},
  _isHidden{false},
  _phase{phase},
  _flash{flash}
{
  _value.reserve(_text.length());
  if(!_phase)
    _value = _text;
}

HUD::IntLabel::IntLabel(Vector2i p, Color3f c, const int32_t* s, int32_t precision, float activeDelay, bool flash) :
  _position{p},
  _color{c},
  _source{s},
  _precision{precision},
  _value{-1},
  _text{std::to_string(_value)},
  _activeDelay{activeDelay},
  _activeTime{0},
  _isActive{false},
  _isVisible{true},
  _isHidden{false},
  _flash{flash}
{}

HUD::BitmapLabel::BitmapLabel(Vector2i p, Color3f c, const Bitmap* b, float activeDelay, bool flash) :
  _position{p},
  _color{c},
  _bitmap{b},
  _activeDelay{activeDelay},
  _activeTime{0},
  _isActive{false},
  _isVisible{true},
  _isHidden{false},
  _flash{flash}
{}

void HUD::initialize(const Font* font, float flashPeriod, float phasePeriod)
{
  _font = font;
  _nextUid = 0;
  _flashPeriod = flashPeriod;
  _phasePeriod = phasePeriod;
  _masterClock = 0.f;
  _flashClock = 0.f;
  _phaseClock = 0.f;
}

HUD::uid_t HUD::addTextLabel(TextLabel label)
{
  label._uid = _nextUid++;
  label._activeTime = _masterClock + label._activeDelay;
  _textLabels.push_back(std::move(label));
  return label._uid;
}

HUD::uid_t HUD::addIntLabel(IntLabel label)
{
  label._uid = _nextUid++;
  label._activeTime = _masterClock + label._activeDelay;
  _intLabels.push_back(std::move(label));
  return label._uid;
}

HUD::uid_t HUD::addBitmapLabel(BitmapLabel label)
{
  label._uid = _nextUid++;
  label._activeTime = _masterClock + label._activeDelay;
  _bitmapLabels.push_back(std::move(label));
  return label._uid;
}

bool HUD::removeTextLabel(uid_t uid)
{
  for(auto iter = _textLabels.begin(); iter != _textLabels.end(); ++iter){
    if((*iter)._uid == uid){
      _textLabels.erase(iter);
      return true;
    }
  }
  return false;
}

bool HUD::removeIntLabel(uid_t uid)
{
  for(auto iter = _intLabels.begin(); iter != _intLabels.end(); ++iter){
    if((*iter)._uid == uid){
      _intLabels.erase(iter);
      return true;
    }
  }
  return false;
}

bool HUD::removeBitmapLabel(uid_t uid)
{
  for(auto iter = _bitmapLabels.begin(); iter != _bitmapLabels.end(); ++iter){
    if((*iter)._uid == uid){
      _bitmapLabels.erase(iter);
      return true;
    }
  }
  return false;
}

void HUD::clear()
{
  _textLabels.clear();
  _intLabels.clear();
  _bitmapLabels.clear();
  _nextUid = 0;
}

void HUD::hideTextLabel(uid_t uid)
{
  for(auto& label : _textLabels)
    if(label._uid == uid)
      label._isHidden = true;
}

void HUD::hideIntLabel(uid_t uid)
{
  for(auto& label : _intLabels)
    if(label._uid == uid)
      label._isHidden = true;
}

void HUD::hideBitmapLabel(uid_t uid)
{
  for(auto& label : _bitmapLabels)
    if(label._uid == uid)
      label._isHidden = true;
}

void HUD::unhideTextLabel(uid_t uid)
{
  for(auto& label : _textLabels)
    if(label._uid == uid)
      label._isHidden = false;
}

void HUD::unhideIntLabel(uid_t uid)
{
  for(auto& label : _intLabels)
    if(label._uid == uid)
      label._isHidden = false;
}

void HUD::unhideBitmapLabel(uid_t uid)
{
  for(auto& label : _bitmapLabels)
    if(label._uid == uid)
      label._isHidden = false;
}

void HUD::startTextLabelFlash(uid_t uid)
{
  for(auto& label : _textLabels)
    if(label._uid == uid)
      label._flash = true;
}

void HUD::startIntLabelFlash(uid_t uid)
{
  for(auto& label : _intLabels)
    if(label._uid == uid)
      label._flash = true;
}

void HUD::startBitmapLabelFlash(uid_t uid)
{
  for(auto& label : _bitmapLabels)
    if(label._uid == uid)
      label._flash = true;
}

void HUD::stopTextLabelFlash(uid_t uid)
{
  for(auto& label : _textLabels)
    if(label._uid == uid)
      label._flash = false;
}

void HUD::stopIntLabelFlash(uid_t uid)
{
  for(auto& label : _intLabels)
    if(label._uid == uid)
      label._flash = false;
}

void HUD::stopBitmapLabelFlash(uid_t uid)
{
  for(auto& label : _bitmapLabels)
    if(label._uid == uid)
      label._flash = false;
}

void HUD::onReset()
{
  for(auto& label : _textLabels){
    label._charNo = 0;
    label._activeTime = label._activeDelay;
    label._isActive = false;
    label._isVisible = true;
    label._isHidden = false;
    if(label._phase)
      label._value = std::string{};
  }

  for(auto& label : _intLabels){
    label._value = *(label._source);
    label._text = std::to_string(label._value);
    label._activeTime = label._activeDelay;
    label._isActive = false;
    label._isVisible = true;
    label._isHidden = false;
  }

  for(auto& label : _bitmapLabels){
    label._activeTime = label._activeDelay;
    label._isActive = false;
    label._isVisible = true;
    label._isHidden = false;
  }

  _masterClock = 0.f;
  _phaseClock = 0.f;
  _flashClock = 0.f;
}

void HUD::onUpdate(float dt)
{
  _masterClock += dt;

  _flashClock += dt;
  if(_flashClock >= _flashPeriod){
    flashLabels();
    _flashClock = 0.f;
  }

  _phaseClock += dt;
  if(_phaseClock >= _phasePeriod){
    phaseLabels();
    _phaseClock = 0.f;
  }

  updateIntLabels();
  activateLabels();
}

void HUD::onDraw()
{
  for(auto& label : _textLabels)
    if(label._isActive && label._isVisible && !label._isHidden)
      nomad::renderer->blitText(label._position, label._value, *_font, label._color);

  for(auto& label : _intLabels){
    if(label._isActive && label._isVisible && !label._isHidden)
      nomad::renderer->blitText(label._position, label._text, *_font, label._color);
  }

  for(auto& label : _bitmapLabels)
    if(label._isActive && label._isVisible && !label._isHidden)
      nomad::renderer->blitBitmap(label._position, *(label._bitmap), label._color);
}

void HUD::flashLabels()
{
  for(auto& label : _textLabels)
    if(label._isActive && label._flash)
      label._isVisible = !label._isVisible;

  for(auto& label : _intLabels)
    if(label._isActive && label._flash)
      label._isVisible = !label._isVisible;

  for(auto& label : _bitmapLabels)
    if(label._isActive && label._flash)
      label._isVisible = !label._isVisible;
}

void HUD::phaseLabels()
{
  for(auto& label : _textLabels){
    if(label._isActive && label._phase && label._charNo != label._text.length()){
      label._value += label._text[label._charNo];
      label._charNo++;
    }
  }
}

void HUD::updateIntLabels()
{
  for(auto& label : _intLabels){
    if(label._isActive && label._value != *(label._source)){
      label._value = *(label._source);
      std::string text = std::to_string(label._value); 
      label._text = std::string{};
      for(int i = 0; i < label._precision - text.length(); ++i)
        label._text += '0';
      label._text += text;
    }
  }
}

void HUD::activateLabels()
{
  for(auto& label : _textLabels)
    if(!label._isActive && label._activeTime < _masterClock)
      label._isActive = true;

  for(auto& label : _intLabels)
    if(!label._isActive && label._activeTime < _masterClock)
      label._isActive = true;

  for(auto& label : _bitmapLabels)
    if(!label._isActive && label._activeTime < _masterClock)
      label._isActive = true;
}

TextInput::TextInput(Font* font, std::string label, Color3f cursorColor)
{

}

const char* TextInput::processInput()
{
  bool isDone {false};
  auto& keyHistory = input->getHistory();
  for(auto key : keyHistory){
    if(key == Input::KEY_RIGHT){
      ++_cursorPos;
      if(_cursorPos >= _bufferSize)
        _cursorPos = _bufferSize - 1;
    }
    else if(key == Input::KEY_LEFT){
      --_cursorPos;
      if(_cursorPos < 0)
        _cursorPos = 0;
    }
    else if(key == Input::KEY_ENTER){
      isDone = true;
      break;
    }
    else if(key == Input::KEY_BACKSPACE){

      //
      // TODO TODO
      //

    }
    else if(Input::KEY_a <= key && key <= Input::KEY_z){
      _buffer[_cursorPos] = input->keyToAsciiCode(key);
      ++_cursorPos;
      if(_cursorPos >= _bufferSize)
        _cursorPos = _bufferSize - 1;
    }
  }
  return (isDone) ? _buffer.data() : nullptr;
}

