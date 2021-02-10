#ifndef _PIXIRETRO_PARTICLE_ENGINE_H_
#define _PIXIRETRO_PARTICLE_ENGINE_H_

#include <array>
#include "math.h"

namespace pxr
{

class HomogenousParticleEngine
{
public:
  HomogenousParticleEngine(int maxParticleCount);

  void spawnParticle(

private:
  struct Particle
  {
    Vector2f _position;
    Vector2f _velocity;
    Vector2f _acceleration;
  };

private:
  std::vector<Particle> _particles;
  Color4u _particleColor;  // color of all particles.
  float _inverseMass;      // inverse mass of all particles.
  int _maxParticleCount;
};

} // namespace pxr 

#endif
