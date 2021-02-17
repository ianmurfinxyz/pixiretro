#ifndef _PIXIRETRO_MATH_RAND_H_
#define _PIXIRETRO_MATH_RAND_H_

#include <random>

namespace pxr
{

template<typename T, typename Dist>
class Rand
{
public:
  Rand(T lo, T hi) : d{lo, hi}
  {
    std::random_device r;
    e.seed(r());
  }

  Rand(const Rand&) = delete;
  Rand(Rand&&) = delete;
  Rand& operator=(const Rand&) = delete;
  Rand& operator=(Rand&&) = delete;

  T operator()() {return d(e);}

private:
    std::default_random_engine e;
    Dist d;
};

using iRand = Rand<int, std::uniform_int_distribution<int>>;
using fRand = Rand<float, std::uniform_real_distribution<float>>;
using dRand = Rand<double, std::uniform_real_distribution<double>>;

} // namespace pxr

#endif
