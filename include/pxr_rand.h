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

  Rand(const RandBasic&) = delete;
  Rand(RandBasic&&) = delete;
  Rand& operator=(const RandBasic&) = delete;
  Rand& operator=(RandBasic&&) = delete;

  T operator()() {return d(e);}

private:
    std::default_random_engine e;
    Dist d;
};

using iRand = RandBasic<int, std::uniform_int_distribution<int>>;
using fRand = RandBasic<float, std::uniform_real_distribution<float>>;
using dRand = RandBasic<double, std::uniform_real_distribution<double>>;

} // namespace pxr

#endif
