class Application;

class ApplicationState
{
public:
  ApplicationState(Application* app) : _app(app) {}
  virtual ~ApplicationState() = default;
  virtual void initialize(Vector2i worldSize, int32_t worldScale) = 0;
  virtual void onUpdate(double now, float dt) = 0;
  virtual void onDraw(double now, float dt) = 0;
  virtual void onReset() = 0;
  virtual std::string getName() = 0;

protected:
  Application* _app;
};

class Engine;

class Application
{
public:
  static constexpr Input::KeyCode pauseKey {Input::KEY_p};

public:
  Application() = default;
  virtual ~Application() = default;
  
  virtual bool initialize(Engine* engine, int32_t windowWidth, int32_t windowHeight);

  virtual std::string getName() const = 0;
  virtual int32_t getVersionMajor() const = 0;
  virtual int32_t getVersionMinor() const = 0;

  void onWindowResize(int32_t windowWidth, int32_t windowHeight);
  virtual void onUpdate(double now, float dt);
  virtual void onDraw(double now, float dt);

  void switchState(const std::string& name);

  bool isWindowTooSmall() const {return _isWindowTooSmall;}

protected:
  virtual Vector2i getWorldSize() const = 0;

  void addState(std::unique_ptr<ApplicationState>&& state);

private:
  void drawWindowTooSmall();

private:
  std::unordered_map<std::string, std::unique_ptr<ApplicationState>> _states;
  std::unique_ptr<ApplicationState>* _activeState;
  iRect _viewport;
  Engine* _engine;
  bool _isWindowTooSmall;
};

