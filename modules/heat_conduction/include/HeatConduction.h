#ifndef HEATCONDUCTIONKERNEL_H
#define HEATCONDUCTIONKERNEL_H

#include "Diffusion.h"
#include "Material.h"

/**
 * Note: This class is named HeatConductionKernel instead of HeatConduction
 * to avoid a class with the namespace HeatConduction.  It is registered
 * as HeatConduction, which means it can be used by that name in the input
 * file.
 */
//Forward Declarations
class HeatConductionKernel;

template<>
InputParameters validParams<HeatConductionKernel>();

class HeatConductionKernel : public Diffusion
{
public:

  HeatConductionKernel(const std::string & name, InputParameters parameters);

protected:
  virtual Real computeQpResidual();

  virtual Real computeQpJacobian();

private:
  const unsigned _dim;
  MaterialProperty<Real> & _diffusion_coefficient;
  MaterialProperty<Real> * const _diffusion_coefficient_dT;

};
#endif //HEATCONDUCTIONKERNEL_H
