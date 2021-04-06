#include <cassert>
#include "pxr_hud.h"

namespace pxr
{

HUD::Label::Label(
  Vector2f position,
  gfx::Color4u color,
  float activationDelay,
  float lifetime) 
  :
  _owner{nullptr},
  _uid{0},
  _position{position},
  _color{color},
  _activationDelay{std::max(0.f, activationDelay)},
  _activationClock{0.f},
  _age{0.f},
  _lifetime{std::max(0.f, lifetime)},
  _flashState{false},
  _isActive{_activationDelay == 0.f},
  _isHidden{false},
  _isFlashing{false},
  _isImmortal(_lifetime == 0.f),
  _isDead{false}
{}

void HUD::Label::onReset()
{
  _activationClock = 0.f;
  _isActive = (_activationDelay == 0.f);
}

void HUD::Label::onUpdate(float dt)
{
  if(!_isActive){
    _activationClock += dt;
    if(_activationClock > _activationDelay)
      _isActive = true;
  }

  if(!_isImmortal){
    _age += dt;
    if(_age > _lifetime)
      _isDead = true;
  }

  if(_isFlashing && _lastFlashNo != _owner->getFlashNo()){
    _lastFlashNo = _owner->getFlashNo();
    _flashState = !_flashState;
  }
}

void HUD::Label::startFlashing()
{
  _isFlashing = true;
  _lastFlashNo = _owner->getFlashNo();
}

void HUD::Label::stopFlashing()
{
  _isFlashing = false;
}

void HUD::Label::initialize(const HUD* owner, uid_t uid)
{
  _owner = owner;
  _uid = uid;
}

HUD::TextLabel::TextLabel(
  Vector2f position,
  gfx::Color4u color,
  float activationDelay,
  float lifetime,
  std::string text,
  bool phaseIn,
  gfx::ResourceKey_t fontKey)
  :
  Label(position, color, activationDelay, lifetime),
  _fontKey{fontKey},
  _fullText{text},
  _visibleText{phaseIn ? text : std::string{}},
  _nextCharToShow{0},
  _isPhasingIn{phaseIn}
{}

void HUD::TextLabel::onReset()
{
  Label::onReset();

  if(_isPhasingIn){
    _visibleText.clear();
    _nextCharToShow = 0;
  }
}

void HUD::TextLabel::onUpdate(float dt)
{
  Label::onUpdate(dt);

  if(!_isActive) return;

  if(_isPhasingIn && _nextCharToShow < _fullText.length() && _lastPhaseInNo != _owner->getPhaseInNo()){
    _lastPhaseInNo = _owner->getPhaseInNo(); 
    _visibleText += _fullText[_nextCharToShow];
    ++_nextCharToShow;
  }
}

void HUD::TextLabel::onDraw(gfx::ScreenID_t screenid)
{
  if(_isActive && !_isHidden && _flashState)
    gfx::drawText(_position, _visibleText, _fontKey, _color, screenid);
}

HUD::IntLabel::IntLabel(
  Vector2f position,
  gfx::Color4u color,
  float activationDelay,
  float lifetime,
  const int& sourceValue,
  int precision,
  gfx::ResourceKey_t fontKey)
  :
  Label(position, color, activationDelay, lifetime),
  _fontKey{fontKey},
  _sourceValue{sourceValue},
  _precision{precision}
{}

void HUD::IntLabel::onReset()
{
  Label::onReset();
}

void HUD::IntLabel::onUpdate(float dt)
{
  Label::onUpdate(dt);

  if(!_isActive) return;

  if(_displayValue != _sourceValue){
    std::string valueStr = std::to_string(_sourceValue);
    _displayStr.clear();
    int sign{0};
    if(_sourceValue < 0){
      assert(valueStr.front() == '-');
      sign = 1;
      _displayStr += '-';
    }
    for(int i{0}; i < _precision - (valueStr.length() - sign); ++i){
      _displayStr += '0';
    }
    _displayStr.append(valueStr.begin() + sign, valueStr.end());
    _displayValue = _sourceValue;
  }
}

void HUD::IntLabel::onDraw(gfx::ScreenID_t screenid)
{
  if(_isActive && !_isHidden && _flashState)
    gfx::drawText(_position, _displayStr, _fontKey, _color, screenid);
}

HUD::BitmapLabel::BitmapLabel(
  Vector2f position,
  gfx::Color4u color,
  float activationDelay,
  float lifetime,
  gfx::ResourceKey_t sheetKey,
  gfx::SpriteID_t spriteid,
  bool mirrorX,
  bool mirrorY)
  :
  Label(position, color, activationDelay, lifetime),
  _sheetKey{sheetKey},
  _spriteid{spriteid},
  _mirrorX{mirrorX},
  _mirrorY{mirrorY}
{}

void HUD::BitmapLabel::onReset()
{
  Label::onReset();
}

void HUD::BitmapLabel::onUpdate(float dt)
{
  Label::onUpdate(dt);
}

void HUD::BitmapLabel::onDraw(gfx::ScreenID_t screenid)
{
  if(_isActive && !_isHidden && _flashState)
    gfx::drawSprite(_position, _sheetKey, _spriteid, screenid, _mirrorX, _mirrorY);
}

HUD::HUD(gfx::ResourceKey_t fontKey, float flashPeriod, float phaseInPeriod) :
  _labels{},
  _fontKey{fontKey},
  _nextUid{0},
  _flashNo{0},
  _phaseInNo{0},
  _flashPeriod{flashPeriod},
  _phaseInPeriod{phaseInPeriod},
  _flashClock{0.f},
  _phaseInClock{0.f}
{}

void HUD::onReset()
{
  for(auto& label : _labels)
    label->onReset();
}

void HUD::onUpdate(float dt)
{
  _flashClock += dt;
  if(_flashClock >= _flashPeriod){
    _flashClock = 0.f;
    ++_flashNo;
  }

  _phaseInClock += dt;
  if(_phaseInClock >= _phaseInPeriod){
    _phaseInClock = 0.f;
    ++_phaseInNo;
  }

  for(auto& label : _labels)
    label->onUpdate(dt);

  _labels.erase(std::remove_if(_labels.begin(), 
                               _labels.end(), 
                               [](const std::unique_ptr<Label>& label){return label->isDead();}),
                _labels.end()
  );
}

void HUD::onDraw(gfx::ScreenID_t screenid)
{
  for(auto& label : _labels)
    label->onDraw(screenid);
}

HUD::uid_t HUD::addLabel(std::unique_ptr<Label> label)
{
  uid_t uid = _nextUid++;
  label->initialize(this, uid);
  _labels.push_back(std::move(label));
  return uid;
}

void HUD::removeLabel(uid_t uid)
{
  _labels.erase(std::remove_if(_labels.begin(), _labels.end(), [uid](const std::unique_ptr<Label>& label){
    return label->getUid() == uid;
  }));
}

void HUD::clear()
{
  _labels.clear();
}

bool HUD::hideLabel(uid_t uid)
{
  auto search = findLabel(uid);
  if(search == _labels.end()) return false;
  (*search)->hide();
  return true;
}

bool HUD::showLabel(uid_t uid)
{
  auto search = findLabel(uid);
  if(search == _labels.end()) return false;
  (*search)->show();
  return true;
}

bool HUD::startLabelFlashing(uid_t uid)
{
  auto search = findLabel(uid);
  if(search == _labels.end()) return false;
  (*search)->startFlashing();
  return true;
}

bool HUD::stopLabelFlashing(uid_t uid)
{
  auto search = findLabel(uid);
  if(search == _labels.end()) return false;
  (*search)->stopFlashing();
  return true;
}

std::vector<std::unique_ptr<HUD::Label>>::iterator HUD::findLabel(uid_t uid)
{
  return std::find_if(_labels.begin(), _labels.end(), [uid](const std::unique_ptr<Label>& label){
    return label->_uid == uid;
  });
}

} // namespace pxr
