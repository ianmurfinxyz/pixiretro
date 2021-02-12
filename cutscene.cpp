#include <cassert>
#include "cutscene.h"
#include "xmlutil.h"
#include "math.h"
#include "gfx.h"
#include "log.h"

//#include <iostream> // TODO remove this

namespace pxr
{
namespace cut
{

Animation::Animation(gfx::ResourceKey_t spriteKey, int startFrame, int layer, float frameFrequency, Mode mode) :
  _mode{mode},
  _spriteKey{spriteKey},
  _frame{startFrame},
  _startFrame{startFrame},
  _layer{layer},
  _frameCount{0},
  _framePeriod{1.f / frameFrequency},
  _frameFrequency{frameFrequency},
  _frameClock{0.f}
{
  assert(STATIC <= _mode && _mode <= RAND);

  _frameCount = gfx::getSpriteFrameCount(_spriteKey);
  assert(startFrame < _frameCount);

  if(_frameFrequency == 0.f)
    _mode = STATIC;
}

void Animation::update(float dt)
{
  if(_mode == STATIC)
    return;

  _frameClock += dt;
  if(_frameClock > _framePeriod){
    if(_mode == LOOP)
      ++_frame;
    else if(_mode == RAND)
      ++_frame;             // TODO: choose random frame

    _frameClock = 0.f;
  }

  if(_frame > _frameCount) 
    _frame = 0;
}

void Animation::reset()
{
  _frame = _startFrame;
  _frameClock = 0.f;
}

Transition::Transition(std::vector<TPoint> points, float duration) :
  _points{std::move(points)},
  _position{0.f, 0.f},
  _duration{duration},
  _clock{0.f},
  _from{0},
  _to{1},
  _isDone{false}
{
  assert(_points.size() != 0);

  for(auto& p : _points)
    std::clamp(p._phase, 0.f, 1.f);

  std::sort(_points.begin(), _points.end(), [](const TPoint& p0, const TPoint& p1){
    return p0._phase <= p1._phase;
  });

  if(_points.size() == 1 || duration == 0.f){
    _to = 0;
    _position = _points.front()._position;
    _isDone = true;
  }
}

void Transition::update(float dt)
{
  if(_isDone)
    return;

  float phase {0.f};

  _clock += dt;
  phase = _clock / _duration;

  if(phase > _points[_to]._phase){
    if(_points[_to]._phase == 1.f){
      _isDone = true;
      phase = 1.f;
    }
    else{
      ++_from;
      ++_to;
    }
  }

  phase -= _points[_from]._phase;
  _position._x = lerp(_points[_from]._position._x, _points[_to]._position._x, phase);
  _position._y = lerp(_points[_from]._position._y, _points[_to]._position._y, phase);
}

void Transition::reset()
{
  _from = 0;
  _to = 1;
  _clock = 0.f;
  _isDone = false;

  if(_points.size() == 1){
    _to = 0;
    _position = _points.front()._position;
    _isDone = true;
  }
}

SceneGraphic::SceneGraphic(Animation animation, Transition transition, float startTime, float duration) :
  _animation{animation},
  _transition{std::move(transition)},
  _startTime{startTime},
  _duration{duration},
  _clock{0.f}
{
  _state = _startTime == 0.f ? ElementState::ACTIVE : ElementState::PENDING;
}

void SceneGraphic::update(float dt)
{
  if(_clock < 0.f)
    _state = ElementState::DONE;

  if(_state == ElementState::DONE)
    return;

  if(_state == ElementState::PENDING){
    _clock += dt;
    if(_clock >= _startTime){
      _state = ElementState::ACTIVE;
      //std::cout << "activated time=" << _clock << "s" << std::endl;
      dt = _clock - _startTime;
      _clock = 0.f;
    }
  }

  if(_state == ElementState::ACTIVE){
    _clock += dt;
    if(_clock >= _duration){
      dt = _clock - _duration;
      //std::cout << "deactivated time=" << _clock << "s" << std::endl;
      _clock = -1.f;
    }
    _animation.update(dt);
    _transition.update(dt);
  }
}

void SceneGraphic::draw(int screenid)
{
  if(_state != ElementState::ACTIVE)
    return;

  gfx::drawSprite(_transition.getPosition(), _animation.getSpriteKey(), _animation.getFrame(), screenid); 
}

void SceneGraphic::reset()
{
  _animation.reset();
  _transition.reset();
  _clock = 0.f;
  _state = _startTime == 0.f ? ElementState::ACTIVE : ElementState::PENDING;
}

SceneSound::SceneSound(sfx::ResourceKey_t soundKey, float startTime, float duration, bool loop) :
  _soundKey{soundKey},
  _startTime{startTime},
  _duration{duration},
  _clock{0.f},
  _loop{loop},
  _state{ElementState::PENDING}
{}

void SceneSound::update(float dt)
{
  if(_state == ElementState::DONE)
    return;

  _clock += dt;
  if(_clock >= _startTime){
    sfx::playSound(_soundKey);
    _state = _loop ? ElementState::ACTIVE : ElementState::DONE;
    return;
  }

  if(_clock >= _startTime + _duration){
    sfx::stopSound(_soundKey);
    _state = ElementState::DONE;
  }
}

void SceneSound::reset()
{
  _state = ElementState::PENDING;
  _clock = 0.f;
}

Cutscene::Cutscene() : 
  _graphics{},
  _sounds{}
{}

Cutscene::~Cutscene()
{
  unload();
}

bool Cutscene::load(std::string name)
{
  log::log(log::INFO, log::msg_cut_loading, name);

  std::string xmlpath{};
  xmlpath += RESOURCE_PATH_CUTSCENES;
  xmlpath += name;
  xmlpath += XML_RESOURCE_EXTENSION_CUTSCENES;
  XMLDocument doc{};
  if(!parseXmlDocument(&doc, xmlpath))
    return false;

  XMLElement* xmlscene{nullptr};
  XMLElement* xmlelement{nullptr};

  if(!extractChildElement(&doc, &xmlscene, "scene")) return false;

  //
  // load graphics.
  //

  std::vector<SceneGraphic> graphics {};

  float timingStart;
  float timingDuration;
  float transitionDuration;
  float transitionPhase;
  float frequency;
  int transitionX;
  int transitionY;
  int startFrame;
  int layer;
  int mode;
  int assetKey;
  const char* assetName {nullptr};

  XMLElement* xmltiming{nullptr};
  XMLElement* xmlanimation{nullptr};
  XMLElement* xmltransition{nullptr};
  XMLElement* xmlpoint{nullptr};

  if(!extractChildElement(xmlscene, &xmlelement, "graphic")) return false;
  do{
    if(!extractChildElement(xmlelement, &xmltiming, "timing")) return false;
    if(!extractFloatAttribute(xmltiming, "start", &timingStart)) return false;
    if(!extractFloatAttribute(xmltiming, "duration", &timingDuration)) return false;

    if(!extractChildElement(xmlelement, &xmlanimation, "animation")) return false;

    if(!extractStringAttribute(xmlanimation, "sprite", &assetName)) return false;
    assetKey = gfx::loadSprite(assetName); 

    if(!extractIntAttribute(xmlanimation, "startframe", &startFrame)) return false;
    if(!extractIntAttribute(xmlanimation, "layer", &layer)) return false;
    if(!extractIntAttribute(xmlanimation, "mode", &mode)) return false;
    if(!extractFloatAttribute(xmlanimation, "frequency", &frequency)) return false;

    if(!extractChildElement(xmlelement, &xmltransition, "transition")) return false;
    if(!extractFloatAttribute(xmltransition, "duration", &transitionDuration)) return false;

    std::vector<Transition::TPoint> _tpoints{};
    if(!extractChildElement(xmltransition, &xmlpoint, "point")) return false;
    do{
      if(!extractIntAttribute(xmlpoint, "x", &transitionX)) return false;
      if(!extractIntAttribute(xmlpoint, "y", &transitionY)) return false;
      if(!extractFloatAttribute(xmlpoint, "phase", &transitionPhase)) return false;
      _tpoints.push_back({Vector2f(transitionX, transitionY), transitionPhase});
      xmlpoint = xmlpoint->NextSiblingElement("point");
    }
    while(xmlpoint != 0);

    Animation animation{assetKey, startFrame, layer, frequency, static_cast<Animation::Mode>(mode)}; 
    Transition transition{std::move(_tpoints), transitionDuration};
    graphics.push_back({animation, std::move(transition), timingStart, timingDuration});
     
    xmlelement = xmlelement->NextSiblingElement("graphic");
  }
  while(xmlelement != 0);

  std::sort(graphics.begin(), graphics.end(), [](const SceneGraphic& e0, const SceneGraphic& e1){
    return e0.getAnimation().getLayer() < e1.getAnimation().getLayer();
  });

  //
  // load sounds.
  //

  std::vector<SceneSound> sounds {};

  int loop {0};

  if(!extractChildElement(xmlscene, &xmlelement, "sound")) return false;
  do{
    if(!extractIntAttribute(xmlelement, "loop", &loop)) return false;
    if(!extractStringAttribute(xmlelement, "name", &assetName)) return false;
    assetKey = sfx::loadSound(assetName); 

    if(!extractChildElement(xmlelement, &xmltiming, "timing")) return false;
    if(!extractFloatAttribute(xmltiming, "start", &timingStart)) return false;
    if(!extractFloatAttribute(xmltiming, "duration", &timingDuration)) return false;

    sounds.push_back(SceneSound{assetKey, timingStart, timingDuration, static_cast<bool>(loop)});

    xmlelement = xmlelement->NextSiblingElement("sound");
  }
  while(xmlelement != 0);

  _graphics = std::move(graphics);
  _sounds = std::move(sounds);

  return true;
}

void Cutscene::unload()
{
  for(auto& graphic : _graphics)
    gfx::unloadSprite(graphic.getAnimation().getSpriteKey());

  _graphics.clear();

  for(auto& sound : _sounds)
    sfx::unloadSound(sound.getSoundKey());

  _sounds.clear();
}

void Cutscene::update(float dt)
{
  for(auto& graphic : _graphics)
    graphic.update(dt);

  for(auto& sound : _sounds)
    sound.update(dt);
}

void Cutscene::draw(int screenid)
{
  for(auto& element : _graphics)
    element.draw(screenid);
}

void Cutscene::reset()
{
  for(auto& graphic : _graphics)
    graphic.reset();

  for(auto& sound : _sounds)
    sound.reset();
}

} // namespace cut
} // namespace pxr
