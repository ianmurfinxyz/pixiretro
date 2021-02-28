#ifndef _PIXIRETRO_ENGINE_H_
#define _PIXIRETRO_ENGINE_H_

#include <SDL2/SDL_keycode.h>

#include <memory>
#include <chrono>

#include "pxr_rc.h"
#include "pxr_app.h"
#include "pxr_color.h"
#include "pxr_gfx.h"
#include "pxr_sfx.h"

namespace pxr
{

//
// The core engine class which manages the main loop, initialisation and shutdown.
//
class Engine final
{
public:
  Engine() = default;
  ~Engine() = default;

  //
  // Call prior to all other methods to setup the engine. Takes an app instance to manage.
  //
  // The engine will initialize the app so it only requires a new instance, not an initialized
  // app instance.
  //
  void initialize(std::unique_ptr<App> app);

  //
  // Call after run() has returned.
  //
  void shutdown();

  //
  // Runs the game loop.
  //
  void run();

private:
  //
  // The system clock and resolution used to record time.
  //
  using Clock_t = std::chrono::steady_clock;
  using TimePoint_t = std::chrono::time_point<Clock_t>;
  using Duration_t = std::chrono::nanoseconds;

  static constexpr Duration_t oneMillisecond {1'000'000     };
  static constexpr Duration_t oneSecond      {1'000'000'000 };
  static constexpr Duration_t oneHalfSecond  {500'000'000   };
  static constexpr Duration_t oneMinute      {60'000'000'000};
  static constexpr Duration_t minFramePeriod {1'000'000     };

  static constexpr float splashDurationSeconds     {1.0f};
  static constexpr float splashWaitDurationSeconds {1.0f};

  static constexpr Vector2i statsScreenResolution {500, 200};
  static constexpr Vector2i pauseScreenResolution {100, 60};

  //
  // Engine reserves this resource name for the font it uses to output engine stats.
  //
  static constexpr gfx::ResourceName_t engineFontName {"dogica8"};

  //
  // Keys used by the engine for user controlled engine features. If these keys
  // clash with your application controls they can be changed here.
  //
  static constexpr int decrementGameClockScaleKey {SDLK_LEFTBRACKET };
  static constexpr int incrementGameClockScaleKey {SDLK_RIGHTBRACKET};
  static constexpr int resetGameClockScaleKey     {SDLK_KP_HASH     };
  static constexpr int pauseGameClockKey          {SDLK_p           };
  static constexpr int toggleDrawEngineStatsKey   {SDLK_BACKQUOTE   };
  static constexpr int skipSplashKey              {SDLK_ESCAPE      };

  //
  // The name of the splash screen assets used by the engine. The engine will attempt 
  // to load the following files:
  //
  //      <gfx::RESOURCE_PATH_SPRITESHEETS>/<splashName>.bmp
  //      <gfx::RESOURCE_PATH_SPRITESHEETS>/<splashName>.spritesheet
  //
  //      <sfx::RESOURCE_PATH_SOUNDS>/<splashName>.wav
  //
  // Failure to load such files will result in the engine skipping the splash screen.
  //
  static constexpr const char* splashName {"pixiretro_splash"};

  //
  // A clock to record the real passage of time.
  //
  class RealClock
  {
  public:
    RealClock() : _start{}, _now{}{}
    void reset(){_now = _start = Clock_t::now();}
    Duration_t update();
    Duration_t getNow() {return _now - _start;}
  private:
    TimePoint_t _start;
    TimePoint_t _now;
  };

  //
  // A clock independent of real time which can be paused and scaled; used as the timeline for 
  // the update tick allowing game time to be slowed and sped up.
  //
  class GameClock
  {
  public:
    GameClock() : _now{}, _scale{1.f}, _isPaused{false}{}
    void update(Duration_t realDt);
    void reset(){_now = Duration_t::zero(); _scale = 1.f; _isPaused = false;}
    Duration_t getNow() const {return _now;}
    void incrementScale(float increment){_scale += increment;}
    void setScale(float scale){_scale = scale;}
    float getScale() const {return _scale;}
    void pause(){_isPaused = true;}
    void unpause(){_isPaused = false;}
    void togglePause(){_isPaused = !_isPaused;}
    bool isPaused() const {return _isPaused;}
  private:
    Duration_t _now;
    float _scale;
    bool _isPaused;
  };

  //
  // A class responsible for invoking a callback at regular (tick) intervals. Used in the
  // gameloop to manage when systems are updated; e.g. game logic and drawing. Results in
  // constant time updates.
  //
  // Can be conceptualised as a timeline in which time is quantised and chases a master 
  // clock. When the time on the master clock exceeds the next quantised time unit on the
  // ticker, the ticker time jumps forward to said unit to catch up. Invoking the callback
  // upon jumping.
  //
  // This class also measures performance statistics regarding the frequency of invocation of
  // its callback. Note that there is a difference between the target frequency and real measured
  // frequency of callback invocation. Further the measured frequency is actually an average 
  // value over the last half second of game time.
  //
  class Ticker
  {
  public:
    //
    // The required signiture of the callback.
    //
    using Callback_t = void (Engine::*)(float);

    //
    // The history is used to plot the performance graph for the ticker thus the size of
    // the history determines the time span the graph covers where each sample is the 
    // average FPS over the last half second. Thus 10 samples = 5 seconds worth of FPS values.
    //
    static constexpr int FPS_HISTORY_SIZE {10};

  public:
    Ticker() = default;
    Ticker(Callback_t onTick, Engine* tickCtx, Duration_t tickPeriod, int maxTicksPerFrame, bool isChasingGameNow);
    void doTicks(Duration_t gameNow, Duration_t realNow);
    void reset();
    int getTicksDoneTotal() const {return _ticksDoneTotal;}
    int getTicksDoneThisFrame() const {return _ticksDoneThisFrame;}
    int getTicksAccumulated() const {return _ticksAccumulated;}
    const std::array<double, FPS_HISTORY_SIZE>& getTickFrequencyHistory() {return _measuredTickFrequencyHistory;}
    bool isNewTickFrequencySample() const {return _isNewTickFrequencySample;}
    void setCallback(Callback_t onTick){_onTick = onTick;}
    
  private:
    Callback_t _onTick;
    Engine* _tickCtx;
    Duration_t _tickerNow;             // current time in the ticker's timeline.
    Duration_t _lastMeasureNow;        // time when tick frequency was last measured.
    Duration_t _tickPeriod;            // ticker timeline is quantised; period of each jump/tick.
    float _tickPeriodSeconds;          // precalculated as passed to callback every tick.
    int _ticksDoneTotal;               // useful performance stat.
    int _ticksDoneThisHalfSecond;      // used to calculate tick frequency.
    int _ticksDoneThisFrame;           // useful performance stat.
    int _maxTicksPerFrame;             // limit to number of ticks in each call to doTicks.
    int _ticksAccumulated;             // backlog of ticks that need to be done.
    bool _isChasingGameNow;            // ticker either 'chases' the real clock or the game clock.

    //
    // Recorded history of samples for ticks per second (analagous to FPS but for ticks). Only 
    // storing the last FPS_HISTORY_SIZE samples with earlier samples being discarded.
    //
    std::array<double, FPS_HISTORY_SIZE> _measuredTickFrequencyHistory; 

    //
    // Flag cleared every frame, thus informs of a new sample only within those frames in which 
    // a call to 'doTicks' generated a new sample.
    //
    bool _isNewTickFrequencySample;
  };

  class EngineRC final : public io::RC
  {
  public:
    static constexpr const char* filename = "engine";
    
    enum Key
    {
      KEY_WINDOW_WIDTH, 
      KEY_WINDOW_HEIGHT, 
      KEY_FULLSCREEN,
      KEY_CLEAR_RED,
      KEY_CLEAR_GREEN,
      KEY_CLEAR_BLUE,
      KEY_FPS_LOCK
    };

    EngineRC() : RC({
      //    key               name        default   min      max
      {KEY_WINDOW_WIDTH,  "windowWidth",  {500},   {300},   {1000}},
      {KEY_WINDOW_HEIGHT, "windowHeight", {500},   {300},   {1000}},
      {KEY_FULLSCREEN,    "fullscreen",   {false}, {false}, {true}},
      {KEY_CLEAR_RED,     "clearRed",     {10},    {0},     {255}},
      {KEY_CLEAR_GREEN,   "clearGreen",   {10},    {0},     {255}},
      {KEY_CLEAR_BLUE,    "clearBlue",    {10},    {0},     {255}},
      {KEY_FPS_LOCK,      "fpsLock",      {60},    {24},    {1000}}
    }){}
  };

private:
  void mainloop();
  void drawEngineStats();
  void drawPauseDialog();
  void onUpdateTick(float tickPeriodSeconds);
  void onDrawTick(float tickPeriodSeconds);

  void splashLoop();
  void onSplashUpdateTick(float tickPeriodSeconds);
  void onSplashDrawTick(float tickPeriodSeconds);
  void onSplashExit();

  double durationToMilliseconds(Duration_t d);
  double durationToSeconds(Duration_t d);
  double durationToMinutes(Duration_t d);
  void durationToDigitalClock(Duration_t d, int& hours, int& mins, int& secs);

private:
  EngineRC _rc;

  Ticker _updateTicker;
  Ticker _drawTicker;

  RealClock _realClock;
  GameClock _gameClock;

  gfx::Color4f _clearColor;

  int _fpsLockHz;

  long _framesDone;
  int _framesDoneThisSecond;
  float _measuredFrameFrequency;
  Duration_t _lastFrameMeasureNow;

  int _statsScreenId;
  int _pauseScreenId;

  gfx::ResourceKey_t _engineFontKey;

  gfx::ResourceKey_t _splashSpriteKey;
  sfx::ResourceKey_t _splashSoundKey;
  Vector2i _splashPosition;
  Vector2i _splashSize;
  int _splashProgress;
  bool _isSplashDone;

  std::unique_ptr<App> _app;

  bool _isDrawingEngineStats;
  bool _needRedrawEngineStats;
  bool _isDone;
};

} // namespace pxr

#endif
