#ifndef _PIXIRETRO_CUTSCENE_H_
#define _PIXIRETRO_CUTSCENE_H_

#include <vector>
#include <string>
#include "gfx.h"

namespace pxr
{
namespace cut
{

static constexpr const char* RESOURCE_PATH_CUTSCENES = "assets/cutscenes/";
static constexpr const char* XML_RESOURCE_EXTENSION_CUTSCENES = ".scene";

class Animation
{
public:
  //
  // The modes are as follows:
  //  STATIC  - a single sprite frame is always shown. Updates do nothing.
  //  LOOP    - the animation loops in ascending order of frame number.
  //  RAND    - the animation will choose a random frame from the sprite every frame change.
  //
  enum Mode { 
    STATIC = 0,     // These enum values are used within .scene files to specify
    LOOP   = 1,     // animation states. Don't change!
    RAND   = 2 
  };

public:
  Animation(gfx::ResourceKey_t spriteKey, int startFrame, int _layer, float frameFrequency, Mode mode);
  bool update(float dt);
  void reset();
  gfx::ResourceKey_t getSpriteKey() const {return _spriteKey;}
  int getFrame() const {return _frame;}
  int getLayer() const {return _layer;}

private:
  Mode _mode;
  gfx::ResourceKey_t _spriteKey;
  int _frame;
  int _startFrame;
  int _frameCount;
  int _layer;
  float _framePeriod;
  float _frameFrequency;
  float _frameClock;
};

class Transition
{
public:
  struct TPoint
  {
    Vector2f _position;
    float _phase;
  };

  Transition(std::vector<TPoint> points, float duration);
  bool update(float dt);
  void reset();
  Vector2f getPosition() const {return _position;}

private:
  std::vector<TPoint> _points;
  Vector2f _position;
  float _duration;
  float _clock;
  int _from;
  int _to;
  bool _isDone;
};

class SceneElement
{
public:
  enum class State { PENDING, ACTIVE, DONE };

  SceneElement(Animation animation, Transition transition, float startTime, float duration);

  bool update(float dt);
  void draw(int screenid);
  void reset();

  State getState() const {return _state;}
  const Animation& getAnimation() const {return _animation;}

private:
  Animation _animation;
  Transition _transition;
  float _startTime;
  float _duration;
  float _clock;
  State _state;
};

class Cutscene
{
private:

public:
  Cutscene() = default;

  //
  // Load a cutscene from a xml cutscene file. Arg 'name' must be the name of file exluding
  // the extension.
  //
  bool load(std::string name);

  void update(float dt);
  void draw(int screenid);
  void reset();

private:
  std::vector<SceneElement> _elements;
  bool _needsRedraw;
};

} // namespace cut
} // namespace pxr

#endif
