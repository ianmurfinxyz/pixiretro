#include <SDL2/SDL.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <cassert>
#include "engine.h"
#include "log.h"
#include "app.h"
#include "input.h"
#include "gfx.h"

#include <iostream>


namespace pxr
{

Engine::Duration_t Engine::RealClock::update()
{
  auto old = _now;
  _now = Clock_t::now();
  return _now - old;
}

void Engine::GameClock::update(Duration_t realDt)
{
  if(!_isPaused)
    _now += Duration_t{static_cast<int64_t>(realDt.count() * _scale)};
}

Engine::Ticker::Ticker(Callback_t onTick, Engine* tickCtx, Duration_t tickPeriod, 
                       int maxTicksPerFrame, bool isChasingGameNow) :
  _onTick{onTick},
  _tickCtx{tickCtx},
  _tickerNow{0},
  _lastMeasureNow{0},
  _tickPeriod{tickPeriod},
  _tickPeriodSeconds{static_cast<double>(tickPeriod.count()) / oneSecond.count()},
  _ticksDoneTotal{0},
  _ticksDoneThisSecond{0},
  _ticksDoneThisFrame{0},
  _maxTicksPerFrame{maxTicksPerFrame},
  _ticksAccumulated{0},
  _measuredTickFrequency{0},
  _isChasingGameNow{isChasingGameNow}
{}

void Engine::Ticker::doTicks(Duration_t gameNow, Duration_t realNow)
{
  Duration_t now = _isChasingGameNow ? gameNow : realNow;

  while(_tickerNow + _tickPeriod < now){
    _tickerNow += _tickPeriod;
    ++_ticksAccumulated;
  }

  _ticksDoneThisFrame = 0;
  while(_ticksAccumulated > 0 && _ticksDoneThisFrame < _maxTicksPerFrame){
    ++_ticksDoneThisFrame;
    --_ticksAccumulated;
    (_tickCtx->*_onTick)(_tickPeriodSeconds);
  }

  _ticksDoneThisSecond += _ticksDoneThisFrame;
  _ticksDoneTotal += _ticksDoneThisFrame;

  if((realNow - _lastMeasureNow) >= oneSecond){
    _measuredTickFrequency = (static_cast<double>(_ticksDoneThisSecond) / 
                             (realNow - _lastMeasureNow).count()) * 
                             oneSecond.count();
    _ticksDoneThisSecond = 0;
    _lastMeasureNow = realNow;
  }
}

void Engine::initialize(std::unique_ptr<App> app)
{
  log::initialize();
  input::initialize();

  if(!_rc.load(EngineRC::filename))
    _rc.write(EngineRC::filename);    // generate a default rc file if one doesn't exist.

  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    log::log(log::FATAL, log::msg_fail_sdl_init, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  _updateTicker = Ticker{&Engine::onUpdateTick, this, fpsLockHz, 1, true};
  _drawTicker = Ticker{&Engine::onDrawTick, this, fpsLockHz, 1, false};

  _app = std::move(app);

  std::stringstream ss {};
  ss << _app->getName() 
     << " version:" 
     << _app->getVersionMajor() 
     << "." 
     << _app->getVersionMinor();

  Vector2i windowSize{};
  windowSize._x = _rc.getIntValue(EngineRC::KEY_WINDOW_WIDTH);
  windowSize._y = _rc.getIntValue(EngineRC::KEY_WINDOW_HEIGHT);
  bool fullscreen = _rc.getBoolValue(EngineRC::KEY_FULLSCREEN);
  gfx::initialize(ss.str(), windowSize, fullscreen);

  gfx::ResourceKey_t fontKey = gfx::loadFont(engineFontName);
  assert(fontKey == engineFontKey);

  int screenid = gfx::createScreen(engineStatsScreenResolution);
  assert(screenid == engineStatsScreenID);
  gfx::setScreenPositionMode(gfx::PositionMode::BOTTOM_LEFT, screenid);
  gfx::setScreenSizeMode(gfx::SizeMode::AUTO_MIN, screenid);

  _app->onInit();

  _framesDone = 0;
  _framesDoneThisSecond = 0;
  _measuredFrameFrequency = 0;
  _lastFrameMeasureNow = Duration_t::zero();
  _isDrawingEngineStats = false;
  _isDone = false;
}

void Engine::shutdown()
{
  gfx::shutdown();
  log::shutdown();
}

void Engine::run()
{
  _realClock.reset();
  while(!_isDone) mainloop();
}

void Engine::mainloop()
{
  auto frameStart = Clock_t::now();

  _gameClock.update(_realClock.update()); 
  auto gameNow = _gameClock.getNow();
  auto realNow = _realClock.getNow();

  SDL_Event event;
  while(SDL_PollEvent(&event) != 0){
    switch(event.type){
      case SDL_QUIT:
        _isDone = true;
        return;
      case SDL_WINDOWEVENT:
        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
          gfx::onWindowResize(Vector2i{event.window.data1, event.window.data2});
        break;
      case SDL_KEYDOWN:
        if(event.key.keysym.sym == decrementGameClockScaleKey){
          _gameClock.incrementScale(-0.1);
          break;
        }
        else if(event.key.keysym.sym == incrementGameClockScaleKey){
          _gameClock.incrementScale(0.1);
          break;
        }
        else if(event.key.keysym.sym == resetGameClockScaleKey){
          _gameClock.setScale(1.f);
          break;
        }
        else if(event.key.keysym.sym == pauseGameClockKey){
          _gameClock.togglePause();
          break;
        }
        else if(event.key.keysym.sym == toggleDrawEngineStatsKey){
          _isDrawingEngineStats = !_isDrawingEngineStats;
          if(!_isDrawingEngineStats)
            gfx::disableScreen(engineStatsScreenID);
          else
            gfx::enableScreen(engineStatsScreenID);
          break;
        }
        // FALLTHROUGH
      case SDL_KEYUP:
        input::onKeyEvent(event);
        break;
    }
  }

  _updateTicker.doTicks(gameNow, realNow);

  //auto now0 = std::chrono::high_resolution_clock::now();
  _drawTicker.doTicks(gameNow, realNow);
  //auto now1 = std::chrono::high_resolution_clock::now();
  //auto dt = std::chrono::duration_cast<std::chrono::microseconds>(now1 - now0);
  //std::cout << "drawTicker.doTicks time: " << dt.count() << "us " << std::endl;

  ++_framesDone;
  ++_framesDoneThisSecond;
  if((realNow - _lastFrameMeasureNow) >= oneSecond){
    _measuredFrameFrequency = (static_cast<double>(_framesDoneThisSecond) / 
                              (realNow - _lastFrameMeasureNow).count()) * 
                              oneSecond.count();
    _lastFrameMeasureNow = realNow;
    _framesDoneThisSecond = 0;
  }

  auto framePeriod = Clock_t::now() - frameStart;
  if(framePeriod < minFramePeriod)
    std::this_thread::sleep_for(minFramePeriod - framePeriod); 
}

void Engine::drawEngineStats()
{
  gfx::clearScreenTransparent(engineStatsScreenID);

  std::stringstream ss{};

  ss << std::setprecision(3);
  ss << "update FPS: " << _updateTicker.getMeasuredTickFrequency() << "hz  "
     << "render FPS: " << _drawTicker.getMeasuredTickFrequency() << "hz  "
     << "frame FPS: " << _measuredFrameFrequency << "hz";
  gfx::drawText({10, 20}, ss.str(), engineFontKey, engineStatsScreenID);

  std::stringstream().swap(ss);

  ss << std::setprecision(3);
  ss << "game time: " << durationToMinutes(_gameClock.getNow()) << "mins  "
     << "real time: " << durationToMinutes(_realClock.getNow()) << "mins";
  gfx::drawText({10, 10}, ss.str(), engineFontKey, engineStatsScreenID);
  
  //std::cout << "update FPS: " << _updateTicker.getMeasuredTickFrequency() << "hz  " << std::endl;
  //std::cout << "render FPS: " << _drawTicker.getMeasuredTickFrequency() << "hz  " << std::endl;
  //std::cout << "frame FPS: " << _measuredFrameFrequency << "hz" << std::endl;
}

void Engine::drawPauseDialog()
{
}

void Engine::onUpdateTick(float tickPeriodSeconds)
{
  double nowSeconds = durationToSeconds(_gameClock.getNow());
  _app->onUpdate(nowSeconds, tickPeriodSeconds);
  input::onUpdate();
}

void Engine::onDrawTick(float tickPeriodSeconds)
{

  //auto now0 = std::chrono::high_resolution_clock::now();
  gfx::clearWindowColor(gfx::colors::black);
  //auto now1 = std::chrono::high_resolution_clock::now();
  //auto dt = std::chrono::duration_cast<std::chrono::microseconds>(now1 - now0);
  //std::cout << "clearWindow time: " << dt.count() << "us " << std::endl;

  double nowSeconds = durationToSeconds(_gameClock.getNow());
  _app->onDraw(nowSeconds, tickPeriodSeconds);

  if(_gameClock.isPaused())
    drawPauseDialog();

  if(_isDrawingEngineStats)
    drawEngineStats();

  gfx::present();

}

double Engine::durationToMilliseconds(Duration_t d)
{
  return static_cast<double>(d.count()) / static_cast<double>(oneMillisecond.count());
}

double Engine::durationToSeconds(Duration_t d)
{
  return static_cast<double>(d.count()) / static_cast<double>(oneSecond.count());
}

double Engine::durationToMinutes(Duration_t d)
{
  return static_cast<double>(d.count()) / static_cast<double>(oneMinute.count());
}

} // namespace pxr

