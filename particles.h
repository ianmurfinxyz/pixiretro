#ifndef _PIXIRETRO_PARTICLE_ENGINE_H_
#define _PIXIRETRO_PARTICLE_ENGINE_H_

#include <array>
#include "math.h"
#include "color.h"

namespace pxr
{

class ParticleEngine
{
public:
  static constexpr int MAX_PARTICLE_COUNT {1000};

  struct RandRange
  {
    float _lo;
    float _hi;
  };

public:
  ParticleEngine(gfx::Color4u color, RandRange velocityRange, RandRange accelerationRange, 
                 float damping = 0.99f);

  ~ParticleEngine() = default;

  void update(float dt);
  void draw(int screenid);

  //
  // Spawns a particle. The version of this function called determines whether the particle
  // is assigned a random velocity and/or acceleration; if the function doesn't take the
  // argument the argument is randomised.
  //
  void spawnParticle(Vector2f position, Vector2f velocity, Vector2f acceleration);
  void spawnParticle(Vector2f position, Vector2f velocity);
  void spawnParticle(Vector2f position);

  void setColor(gfx::Color4u color) {_color = color;}
  void setDamping(float damping) {_damping = std::clamp(damping, 0.f, 1.f);}
  const gfx::Color4u& getParticleColor() const {return _color;}
  float getDamping() const {return _damping;}

private:
  struct Particle
  {
    Vector2f _position;
    Vector2f _velocity;
    Vector2f _acceleration;
  };

private:
  std::vector<Particle> _particles;
  RandReal _velRand;
  RandReal _accRand;
  gfx::Color4u _color; 
  float _damping;
};

} // namespace pxr 

#endif
