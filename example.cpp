#include <memory>
#include <cassert>
#include "engine.h"
#include "app.h"
#include "gfx.h"
#include "math.h"
#include "cutscene.h"

#include <iostream>

enum SpriteKey
{
  SPKEY_BLUE_DOUBLE_LADDER           = 0,
  SPKEY_BLUE_SINGLE_LADDER           = 1,
  SPKEY_RED_GIRDER_LONG_FLAT         = 2, 
  SPKEY_RED_GIRDER_LONG_RIGHT_SLOPE  = 3,
  SPKEY_RED_GIRDER_LONG_LEFT_SLOPE   = 4,
  SPKEY_RED_GIRDER_TOP_SLOPE         = 5,
  SPKEY_RED_GIRDER_BOTTOM_SLOPE      = 6,
  SPKEY_RED_GIRDER_BOTTOM_FLAT       = 7,
  SPKEY_RED_GIRDER_SHORT             = 8,
  SPKEY_KONG_CLIMB_PAULINE           = 9,
  SPKEY_OIL_BARREL                   = 10,
  SPKEY_OIL_FLAMES                   = 11,
};

enum CutsceneKey
{
  CUTSKEY_INTRO = 0,
};

static std::vector<pxr::cut::Cutscene> cutscenes;

static constexpr int STAGE_SCREEN_ID {1};
static constexpr pxr::Vector2i WORLD_SIZE {224, 256};

class SplashState final : public pxr::AppState
{
public:
  static constexpr const char* name {"splash"};

public:
  SplashState(pxr::App* owner) : AppState(owner){}

  bool onInit()
  {
  }

  void onUpdate(double now, float dt)
  {
    cutscenes[CUTSKEY_INTRO].update(dt);
  }

  void onDraw(double now, float dt)
  {
    pxr::gfx::clearScreenShade(20, STAGE_SCREEN_ID);

    cutscenes[CUTSKEY_INTRO].draw(STAGE_SCREEN_ID);


    //pxr::gfx::drawSprite(_alienPosition, SPRITEID_SQUID, 0, STAGE_SCREEN_ID);
    //pxr::gfx::drawSprite(pxr::Vector2i{10, 10}, SPRITEID_CRAB, 0, STAGE_SCREEN_ID);
    //pxr::gfx::drawText(pxr::Vector2i{-20, 100}, "hello world", 0, pxr::gfx::LAYER_UI);
    //pxr::gfx::drawText(pxr::Vector2i{20, 130}, "!\"#$%&`()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, STAGE_SCREEN_ID);
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

    int screenid = pxr::gfx::createScreen(WORLD_SIZE);
    assert(screenid == STAGE_SCREEN_ID);

    pxr::gfx::ResourceKey_t rkey; 
    rkey = pxr::gfx::loadSprite("blue_double_ladder");
    assert(rkey == SPKEY_BLUE_DOUBLE_LADDER);
    rkey = pxr::gfx::loadSprite("blue_single_ladder");
    assert(rkey == SPKEY_BLUE_SINGLE_LADDER);
    rkey = pxr::gfx::loadSprite("red_girder_long_flat");
    assert(rkey == SPKEY_RED_GIRDER_LONG_FLAT);
    rkey = pxr::gfx::loadSprite("red_girder_long_right_slope");
    assert(rkey == SPKEY_RED_GIRDER_LONG_RIGHT_SLOPE);
    rkey = pxr::gfx::loadSprite("red_girder_long_left_slope");
    assert(rkey == SPKEY_RED_GIRDER_LONG_LEFT_SLOPE);
    rkey = pxr::gfx::loadSprite("red_girder_top_slope");
    assert(rkey == SPKEY_RED_GIRDER_TOP_SLOPE);
    rkey = pxr::gfx::loadSprite("red_girder_bottom_slope");
    assert(rkey == SPKEY_RED_GIRDER_BOTTOM_SLOPE);
    rkey = pxr::gfx::loadSprite("red_girder_bottom_flat");
    assert(rkey == SPKEY_RED_GIRDER_BOTTOM_FLAT);
    rkey = pxr::gfx::loadSprite("red_girder_short");
    assert(rkey == SPKEY_RED_GIRDER_SHORT);
    rkey = pxr::gfx::loadSprite("kong_climb_pauline");
    assert(rkey == SPKEY_KONG_CLIMB_PAULINE);
    rkey = pxr::gfx::loadSprite("oil_barrel");
    assert(rkey == SPKEY_OIL_BARREL);
    rkey = pxr::gfx::loadSprite("oil_flames");
    assert(rkey == SPKEY_OIL_FLAMES);

    pxr::cut::Cutscene cutscene{};
    bool result = cutscene.load("intro");
    assert(result == true);
    cutscenes.push_back(std::move(cutscene));
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
