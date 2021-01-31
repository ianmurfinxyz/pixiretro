#include <memory>
#include <cassert>
#include "engine.h"
#include "app.h"
#include "gfx.h"
#include "math.h"

#include <iostream>

static constexpr int STAGE_SCREEN_ID {1};
static constexpr pxr::Vector2i worldSize {80, 50};

enum SpriteID
{
  SPRITEID_SQUID,
  SPRITEID_CRAB
};

class SplashState final : public pxr::AppState
{
public:
  static constexpr const char* name {"splash"};

public:
  SplashState(pxr::App* owner) : AppState(owner){}

  bool onInit()
  {
    _alienPosition = pxr::Vector2i{10, 10};
    _alienSpeedX = 40;
  }

  void onUpdate(double now, float dt)
  {
    _alienPosition._x += _alienSpeedX * dt;
    if(_alienPosition._x < 0 || _alienPosition._x > (worldSize._x - 20))
      _alienSpeedX *= -1;
  }

  void onDraw(double now, float dt)
  {
    pxr::gfx::clearScreenShade(20, STAGE_SCREEN_ID);
    pxr::gfx::drawSprite(_alienPosition, SPRITEID_SQUID, 0, STAGE_SCREEN_ID);
    pxr::gfx::drawSprite(pxr::Vector2i{10, 10}, SPRITEID_CRAB, 0, STAGE_SCREEN_ID);
    //pxr::gfx::drawText(pxr::Vector2i{-20, 100}, "hello world", 0, pxr::gfx::LAYER_UI);
    //pxr::gfx::drawText(pxr::Vector2i{20, 130}, "!\"#$%&`()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, pxr::gfx::LAYER_UI);
    //pxr::gfx::drawText(pxr::Vector2i{20, 120}, "[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~", 0, pxr::gfx::LAYER_UI);
    //pxr::gfx::drawText(pxr::Vector2i{20, 140}, "\"this is a quote\"", 0, pxr::gfx::LAYER_UI);
    //pxr::gfx::drawText(pxr::Vector2i{20, 150}, "$cat file.txt >> main.cpp -la", 0, pxr::gfx::LAYER_UI);
  }

  void onReset()
  {

  }

  std::string getName() const {return name;}

private:
  pxr::Vector2f _alienPosition;
  float _alienSpeedX; // px per seconds
};


class ExampleApp final : public pxr::App
{
public:
  static constexpr int versionMajor {1};
  static constexpr int versionMinor {0};
  static constexpr const char* name {"example-app"};

public:
  ExampleApp() = default;
  ~ExampleApp() = default;

  bool onInit()
  {
    _active = std::shared_ptr<pxr::AppState>(new SplashState(this));
    _active->onInit();
    _states.emplace(_active->getName(), _active);

    int screenid = pxr::gfx::createScreen(worldSize);
    assert(screenid == STAGE_SCREEN_ID);

    pxr::gfx::ResourceKey_t rkey = pxr::gfx::loadSprite("squid");
    assert(rkey == SPRITEID_SQUID);
    rkey = pxr::gfx::loadSprite("crab");
    assert(rkey == SPRITEID_CRAB);
  }

  virtual std::string getName() const {return std::string{name};}
  virtual int getVersionMajor() const {return versionMajor;}
  virtual int getVersionMinor() const {return versionMinor;}
};

pxr::Engine engine;

int main()
{
  engine.initialize(std::unique_ptr<pxr::App>(new ExampleApp()));
  engine.run();
  engine.shutdown();
}
