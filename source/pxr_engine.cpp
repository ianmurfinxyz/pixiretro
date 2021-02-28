#include <SDL2/SDL.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <cassert>
#include "pxr_engine.h"
#include "pxr_log.h"
#include "pxr_app.h"
#include "pxr_input.h"
#include "pxr_gfx.h"
#include "pxr_sfx.h"
#include "pxr_color.h"

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
  _tickPeriodSeconds{static_cast<float>(tickPeriod.count()) / oneSecond.count()},
  _ticksDoneTotal{0},
  _ticksDoneThisHalfSecond{0},
  _ticksDoneThisFrame{0},
  _maxTicksPerFrame{maxTicksPerFrame},
  _ticksAccumulated{0},
  _isChasingGameNow{isChasingGameNow},
  _isNewTickFrequencySample{false}
{
  for(int i = 0; i < FPS_HISTORY_SIZE - 1; ++i)
    _measuredTickFrequencyHistory[i] = 0.0;

}

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

  _ticksDoneThisHalfSecond += _ticksDoneThisFrame;
  _ticksDoneTotal += _ticksDoneThisFrame;

  _isNewTickFrequencySample = false;
  if((realNow - _lastMeasureNow) >= oneHalfSecond){
    double freqSample = (static_cast<double>(_ticksDoneThisHalfSecond) / 
                        (realNow - _lastMeasureNow).count()) * oneHalfSecond.count() * 2;

    for(int i = 0; i < FPS_HISTORY_SIZE - 1; ++i)
      _measuredTickFrequencyHistory[i] = _measuredTickFrequencyHistory[i + 1];

    _measuredTickFrequencyHistory[FPS_HISTORY_SIZE - 1] = freqSample;

    _ticksDoneThisHalfSecond = 0;
    _lastMeasureNow = realNow;
    _isNewTickFrequencySample = true;
  }
}

void Engine::Ticker::reset()
{
  _tickerNow = Duration_t::zero();
  _lastMeasureNow = Duration_t::zero();
  _ticksDoneTotal = 0;
  _ticksDoneThisHalfSecond = 0;
  _ticksDoneThisFrame = 0;
  _ticksAccumulated = 0;
}

void Engine::initialize(std::unique_ptr<App> app)
{
  log::initialize();
  input::initialize();

  if(!_rc.load(EngineRC::filename))
    _rc.write(EngineRC::filename);    // generate a default rc file if one doesn't exist.

  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    log::log(log::FATAL, log::msg_eng_fail_sdl_init, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  if(!sfx::initialize()){
    log::log(log::FATAL, log::msg_sfx_fail_init);
    exit(EXIT_FAILURE);
  }

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
  if(!gfx::initialize(ss.str(), windowSize, fullscreen)){
    log::log(log::FATAL, log::msg_gfx_fail_init);
    exit(EXIT_FAILURE);
  }

  _engineFontKey = gfx::loadFont(engineFontName);
  
  if(!_app->onInit()){
    log::log(log::FATAL, log::msg_eng_fail_init_app);
    exit(EXIT_FAILURE);
  }

  _statsScreenId = gfx::createScreen(statsScreenResolution);
  gfx::setScreenPositionMode(gfx::PositionMode::BOTTOM_LEFT, _statsScreenId);
  gfx::setScreenSizeMode(gfx::SizeMode::AUTO_MIN, _statsScreenId);
  gfx::disableScreen(_statsScreenId);

  _pauseScreenId = gfx::createScreen(pauseScreenResolution);

  _fpsLockHz = _rc.getIntValue(EngineRC::KEY_FPS_LOCK);
  Duration_t tickPeriod {static_cast<int64_t>(1.0e9 / static_cast<double>(_fpsLockHz))};
  log::log(log::INFO, log::msg_eng_locking_fps, std::to_string(_fpsLockHz) + "hz");

  _updateTicker = Ticker{&Engine::onSplashUpdateTick, this, tickPeriod, 1, true};
  _drawTicker = Ticker{&Engine::onSplashDrawTick, this, tickPeriod, 1, false};

  _splashSoundKey = sfx::loadSound(splashName);
  _splashSpriteKey = gfx::loadSpritesheet(splashName);
  if(gfx::isErrorSpritesheet(_splashSpriteKey)){
    log::log(log::ERROR, log::msg_eng_fail_load_splash);
    onSplashExit();
  }
  else{
    _splashSize = gfx::getSpriteSize(_splashSpriteKey, 0);
    _splashPosition = Vector2i{
      ((pauseScreenResolution._x - _splashSize._x) / 2),
      ((pauseScreenResolution._y - _splashSize._y) / 2),
    };
    _splashProgress = 0;
    _isSplashDone = false;
    gfx::enableScreen(_pauseScreenId);
    gfx::setScreenSizeMode(gfx::SizeMode::AUTO_MAX, _pauseScreenId);
  }

  gfx::Color4u clearColor {
    static_cast<uint8_t>(_rc.getIntValue(EngineRC::KEY_CLEAR_RED)),
    static_cast<uint8_t>(_rc.getIntValue(EngineRC::KEY_CLEAR_GREEN)),
    static_cast<uint8_t>(_rc.getIntValue(EngineRC::KEY_CLEAR_BLUE)),
    255
  };

  _clearColor = clearColor;

  _framesDone = 0;
  _framesDoneThisSecond = 0;
  _measuredFrameFrequency = 0;
  _lastFrameMeasureNow = Duration_t::zero();
  _isDrawingEngineStats = false;
  _isDone = false;
}

void Engine::shutdown()
{
  _app->onShutdown();
  gfx::shutdown();
  sfx::shutdown();
  log::shutdown();
}

void Engine::run()
{
  _realClock.reset();
  while(!_isSplashDone) 
    mainloop();
  
  _realClock.reset();
  _gameClock.reset();
  _updateTicker.reset();
  _drawTicker.reset();
  while(!_isDone) 
    mainloop();
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
        _isSplashDone = true;
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
          if(!_isSplashDone)
            continue;
          _gameClock.togglePause();
          if(_gameClock.isPaused())
            gfx::enableScreen(_pauseScreenId);
          else
            gfx::disableScreen(_pauseScreenId);
          break;
        }
        else if(event.key.keysym.sym == toggleDrawEngineStatsKey){
          _isDrawingEngineStats = !_isDrawingEngineStats;
          if(!_isDrawingEngineStats)
            gfx::disableScreen(_statsScreenId);
          else
            gfx::enableScreen(_statsScreenId);
          break;
        }
        else if(event.key.keysym.sym == skipSplashKey && !_isSplashDone){
          onSplashExit(); 
          break;
        }
        // FALLTHROUGH
      case SDL_KEYUP:
        input::onKeyEvent(event);
        break;
    }
  }

  _updateTicker.doTicks(gameNow, realNow);
  _drawTicker.doTicks(gameNow, realNow);

  if(_updateTicker.isNewTickFrequencySample() || _drawTicker.isNewTickFrequencySample())
    _needRedrawEngineStats = true;

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
  if(!_needRedrawEngineStats)
    return;

  gfx::clearScreenShade(1, _statsScreenId);

  const auto& updateHistory = _updateTicker.getTickFrequencyHistory();
  const auto& drawHistory = _drawTicker.getTickFrequencyHistory();

  std::stringstream ss{};

  ss << std::setprecision(3);
  ss << "update FPS: " << updateHistory[Ticker::FPS_HISTORY_SIZE - 1] << "hz  "
     << "render FPS: " << drawHistory[Ticker::FPS_HISTORY_SIZE - 1] << "hz  "
     << "frame FPS: " << _measuredFrameFrequency << "hz";
  gfx::drawText({10, 20}, ss.str(), _engineFontKey, _statsScreenId);

  std::stringstream().swap(ss);

  int gameHours, gameMins, gameSecs, realHours, realMins, realSecs;
  durationToDigitalClock(_gameClock.getNow(), gameHours, gameMins, gameSecs);
  durationToDigitalClock(_realClock.getNow(), realHours, realMins, realSecs);

  ss << std::setprecision(3);
  ss << "time [h:m:s] -- game=" << gameHours << ":" << gameMins << ":" << gameSecs
                 << " -- real=" << realHours << ":" << realMins << ":" << realSecs;
  gfx::drawText({10, 10}, ss.str(), _engineFontKey, _statsScreenId);

  _needRedrawEngineStats = false;
}

void Engine::drawPauseDialog()
{
  static constexpr const char* dialogTxt = "PAUSED";

  gfx::clearScreenShade(1, _pauseScreenId);

  int xmax = pauseScreenResolution._x - 1;
  int ymax = pauseScreenResolution._y - 1;

  gfx::drawLine(Vector2i{0, 0}, Vector2i{0, ymax}, gfx::colors::barbiepink, _pauseScreenId);
  gfx::drawLine(Vector2i{0, 0}, Vector2i{xmax, 0}, gfx::colors::barbiepink, _pauseScreenId);
  gfx::drawLine(Vector2i{0, ymax}, Vector2i{xmax, ymax}, gfx::colors::barbiepink, _pauseScreenId);
  gfx::drawLine(Vector2i{xmax, 0}, Vector2i{xmax, ymax}, gfx::colors::barbiepink, _pauseScreenId);

  Vector2i pausedTxtPos{};
  Vector2i pausedTxtBox = gfx::calculateTextSize(dialogTxt, _engineFontKey);
  pausedTxtPos._x = (xmax / 2) - (pausedTxtBox._x / 2); 
  pausedTxtPos._y = (ymax / 2) - (pausedTxtBox._y / 2);

  gfx::drawText(pausedTxtPos, dialogTxt, _engineFontKey, _pauseScreenId);
}

void Engine::onUpdateTick(float tickPeriodSeconds)
{
  double nowSeconds = durationToSeconds(_gameClock.getNow());
  _app->onUpdate(nowSeconds, tickPeriodSeconds);
  input::onUpdate();
}

void Engine::onDrawTick(float tickPeriodSeconds)
{
  gfx::clearWindowColor(gfx::colors::silver);

  double nowSeconds = durationToSeconds(_gameClock.getNow());
  _app->onDraw(nowSeconds, tickPeriodSeconds);

  if(_isDrawingEngineStats)
    drawEngineStats();

  gfx::present();

}

void Engine::onSplashUpdateTick(float tickPeriodSeconds)
{
  static bool playedSound {false};
  static int splashMode {0}; // 0=waiting, 1=splashing.
  static float clock {0.f};
  static float delay {splashDurationSeconds / _splashSize._x};

  clock += tickPeriodSeconds;

  if(splashMode == 0){
    if(clock > splashWaitDurationSeconds){
      clock = 0.f;
      if(_splashProgress == _splashSize._x){
        onSplashExit();
      }
      else
        splashMode = 1;
    }
  }

  else if(splashMode == 1){
    if(!playedSound){
      sfx::playSound(_splashSoundKey);  
      playedSound = true;
    }
    if(clock > delay){
      clock = 0.f;
      ++_splashProgress;
      if(_splashProgress >= _splashSize._x)
        splashMode = 0;
    }
  }
}

void Engine::onSplashDrawTick(float tickPeriodSeconds)
{
  gfx::clearWindowColor(gfx::colors::silver);
  gfx::clearScreenShade(1, _pauseScreenId);
  
  for(int col = 0; col < _splashProgress; ++col)
    gfx::drawSpriteColumn(_splashPosition, _splashSpriteKey, 0, col, _pauseScreenId);

  if(_isDrawingEngineStats)
    drawEngineStats();

  gfx::present();
}

void Engine::onSplashExit()
{
  _isSplashDone = true;
  _updateTicker.setCallback(&Engine::onUpdateTick);
  _drawTicker.setCallback(&Engine::onDrawTick);
  sfx::unloadSound(_splashSoundKey);
  gfx::disableScreen(_pauseScreenId);
  drawPauseDialog();
  gfx::setScreenSizeMode(gfx::SizeMode::AUTO_MIN, _pauseScreenId);
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

void Engine::durationToDigitalClock(Duration_t d, int& hours, int& mins, int& secs)
{
  int s = std::floor(durationToSeconds(d));
  hours = s / 3600;
  s %= 3600;
  mins = s / 60;
  s %= 60;
  secs = s;
}

} // namespace pxr

