#include "AbaqusCreepMaterial.h"

#include "SymmTensor.h"
#include "Factory.h"

#include <dlfcn.h>
#define QUOTE(macro) stringifyName(macro)

template<>
InputParameters validParams<AbaqusCreepMaterial>()
{
  InputParameters params = validParams<VolumetricModel>();
  params.addRequiredParam<FileName>("plugin", "The path to the compiled dynamic library for the plugin you want to use (without -opt.plugin or -dbg.plugin)");
  params.addRequiredParam<Real>("youngs_modulus", "Young's Modulus");
  params.addRequiredParam<Real>("poissons_ratio", "Poissons Ratio");
  params.addRequiredParam<Real>("num_state_vars", "The number of state variables this CREEP routine will use");
  params.addRequiredParam<unsigned int>("integration_flag", "The creep integration method: Explicit = 0 and Implicit = 1");
  params.addRequiredParam<unsigned int>("solve_definition", "Creep/Swell Explicit/Implicit Integration Definition to use: 1 - 5");
  params.addParam<unsigned int>("routine_flag",0, "The flag determining when the routine is called: Start of increment = 0 and End of Increment = 1");
  return params;
}

AbaqusCreepMaterial::AbaqusCreepMaterial(const std::string  & name,
                                 InputParameters parameters) :
    VolumetricModel( name, parameters ),
    _plugin(getParam<FileName>("plugin")),
    _youngs_modulus(getParam<Real>("youngs_modulus")),
    _poissons_ratio(getParam<Real>("poissons_ratio")),
    _num_state_vars(getParam<Real>("num_state_vars")),
    _integration_flag(getParam<unsigned int>("integration_flag")),
    _solve_definition(getParam<unsigned int>("solve_definition")),
    _routine_flag(getParam<unsigned int>("routine_flag")),
    _state_var(declareProperty<std::vector<Real> >("state_var")),
    _state_var_old(declarePropertyOld<std::vector<Real> >("state_var")),
    _trial_stress(declareProperty<SymmTensor>("trial_stress")),
    _trial_stress_old(declarePropertyOld<SymmTensor>("trial_stress")),
    _dev_trial_stress(declareProperty<SymmTensor>("dev_trial_stress")),
    _dev_trial_stress_old(declarePropertyOld<SymmTensor>("dev_trial_stress")),
    _ets(declareProperty<Real>("effective_trial_stress")),
    _ets_old(declarePropertyOld<Real>("effective_trial_stress")),
    _stress(declareProperty<SymmTensor>("stress")),
    _stress_old(declarePropertyOld<SymmTensor>("stress")),
    _creep_inc(declareProperty<Real>("creep_inc")),
    _creep_inc_old(declarePropertyOld<Real>("creep_inc")),
    _total_creep(declareProperty<Real>("total_creep")),
    _total_creep_old(declarePropertyOld<Real>("total_creep")),
    _swell_inc(declareProperty<Real>("swell_inc")),
    _swell_inc_old(declarePropertyOld<Real>("swell_inc")),
    _total_swell(declareProperty<Real>("total_swell")),
    _total_swell_old(declarePropertyOld<Real>("total_swell"))
{
#if defined(METHOD)
  _plugin += std::string("-") + QUOTE(METHOD) + ".plugin";
#endif

  _STATEV = new Real[_num_state_vars];

  //Set subroutine values
  _NSTATV = _num_state_vars;
  _LEXIMP = _integration_flag;
  _LEND = _routine_flag;

  //Calculate constants needed for elasticity tensor
  _ebulk3 = _youngs_modulus / (1. - 2.*_poissons_ratio);
  _eg2 = _youngs_modulus / (1. + _poissons_ratio);
  _eg = _eg2 / 2.;
  _eg3 = 3. * _eg;
  _elam = (_ebulk3 - _eg2) / 3.;

  _elasticity_tensor[0] = _elam+_eg2;
  _elasticity_tensor[1] = _elam;
  _elasticity_tensor[2] = _eg;

  // Open the library
  _handle = dlopen(_plugin.c_str(), RTLD_LAZY);

  if (!_handle)
  {
    std::ostringstream error;
    error << "Cannot open library: " << dlerror() << '\n';
    mooseError(error.str());
  }

  // Reset errors
  dlerror();

  // Snag the function pointer from the library
  {
    void * pointer = dlsym(_handle, "creep_");
    _creep = *reinterpret_cast<creep_t*>( &pointer );
  }

  // Catch errors
  const char *dlsym_error = dlerror();
  if (dlsym_error)
  {
    dlclose(_handle);
    std::ostringstream error;
    error << "Cannot load symbol 'creep_': " << dlsym_error << '\n';
    mooseError(error.str());
  }
}

AbaqusCreepMaterial::~AbaqusCreepMaterial()
{
  delete _STATEV;

  dlclose(_handle);
}

void AbaqusCreepMaterial::initStatefulProperties(unsigned n_points)
{
  for ( unsigned qp(0); qp < n_points; ++qp )
  {
    //Initialize state variable vector
    _state_var[qp].resize(_num_state_vars);
    _state_var_old[qp].resize(_num_state_vars);
    for (unsigned int i=0; i<_num_state_vars; i++)
    {
      _state_var[qp][i] = 0.0;
      _state_var_old[qp][i] = 0.0;
    }
  }
}

void AbaqusCreepMaterial::modifyStrain(const unsigned int qp,
                                       const Real /*scale_factor*/,
                                       SymmTensor & strain_increment,
                                       SymmTensor & /*dstrain_increment_dT*/)
{
  // Recover "old" state variables
  for (unsigned int i=0; i<_num_state_vars; i++)
    _STATEV[i]=_state_var_old[qp][i];

  // Initialize DECRA and DESWA arrays
  for (unsigned int i=0; i<5; i++)
  {
    _DECRA[i] = 0.0;
    _DESWA[i] = 0.0;
  }

  // Calculate stress array components
  _stress_component[0] =
    (_elasticity_tensor[0]*strain_increment.component(0)) +
    (_elasticity_tensor[1]*strain_increment.component(1)) +
    (_elasticity_tensor[1]*strain_increment.component(2));

  _stress_component[1] =
    (_elasticity_tensor[1]*strain_increment.component(0)) +
    (_elasticity_tensor[0]*strain_increment.component(1)) +
    (_elasticity_tensor[1]*strain_increment.component(2));

  _stress_component[2] =
    (_elasticity_tensor[1]*strain_increment.component(0)) +
    (_elasticity_tensor[1]*strain_increment.component(1)) +
    (_elasticity_tensor[0]*strain_increment.component(2));

  _stress_component[3] = (_elasticity_tensor[2]*strain_increment.component(3));
  _stress_component[4] = (_elasticity_tensor[2]*strain_increment.component(4));
  _stress_component[5] = (_elasticity_tensor[2]*strain_increment.component(5));

  // Calculate trial stress and deviatoric trial stress
  SymmTensor trial_stress(_stress_component[0], _stress_component[1], _stress_component[2],
                          _stress_component[3], _stress_component[4], _stress_component[5]);
  _trial_stress[qp] = trial_stress;
  _trial_stress[qp] += _stress_old[qp];
  _dev_trial_stress[qp] = _trial_stress[qp];
  _dev_trial_stress[qp].addDiag(-_trial_stress[qp].trace()/3.0);

  // Calculate effective trial stress (_ets)
  Real dts_squared = _dev_trial_stress[qp].doubleContraction(_dev_trial_stress[qp]);

  if (dts_squared >= 0.)
    _ets[qp] = std::sqrt(1.5 * dts_squared);
  else
    mooseError("Attempted to take square root of a negative number!\n");

  // Calculate gradient of dev stress potential (grad_dts)
  // grad_dts = d(qtild)/d(dev_trial_stress)
  SymmTensor delta_dts = _dev_trial_stress[qp] - _dev_trial_stress_old[qp];

  Real delta_ets = _ets[qp] - _ets_old[qp];

  Real grad_dts_potential[6];
  for (unsigned int i=0; i<6; i++)
  {
    if (delta_dts.component(i) == 0.0)
      grad_dts_potential[i] = 0.0;
    else
      grad_dts_potential[i] = delta_ets / delta_dts.component(i);
  }

  SymmTensor grad_dts(grad_dts_potential[0], grad_dts_potential[1], grad_dts_potential[2],
                      grad_dts_potential[3], grad_dts_potential[4], grad_dts_potential[5]);

  // Pass variables in for information
  _KSTEP = _t_step;                   //Step number
  _TIME[0] = _dt;                     //Value of step time at the end of the increment - Check
  _TIME[1] = _t;                      //Value of total time at the end of the increment - Check
  _DTIME = _dt;                       //Time increment
  _EC[0] = _total_creep_old[qp];      //Metal and Drucker-Prager creep at the start of the increment
  _EC[1] = _total_creep[qp];          //Metal and Drucker-Prager creep at the end of the increment
  _ESW[0] = _total_swell_old[qp];     //Metal swell at the start of the increment
  _ESW[1] = _total_swell[qp];         //Metal swell at the end of the increment
  _QTILD = _ets[qp];                  //Von mises equivalent stress
  _P = -_trial_stress[qp].trace();    //Equivalent pressure stress

  // Connection to extern statement
  _creep(_DECRA, _DESWA, _STATEV, &_SERD, _EC, _ESW, &_P, &_QTILD, &_TEMP, &_DTEMP, _PREDEF, _DPRED,
         _TIME, &_DTIME, &_CMNAME, &_LEXIMP, &_LEND, _COORDS, &_NSTATV, &_NOEL, &_NPT, &_LAYER, &_KSPT, &_KSTEP, &_KINC);

  // Update state variables
  for (unsigned int i=0; i<_num_state_vars; i++)
    _state_var[qp][i] = _STATEV[i];

  // Solve for Incremental creep (creep_inc_used) and/or Incremental Swell (swell_inc_used) based on definition
  // NOTE: Below are equations for metal creep and/or time-dependent volumetric swelling materials only
  // Drucker-Prager, Capped Drucker-Prager, and Gasket materials have not been included
  _creep_inc[qp] = _DECRA[0];
  _total_creep[qp] = _creep_inc[qp];
  _total_creep[qp] += _total_creep_old[qp];

  _swell_inc[qp] = _DESWA[0];
  _total_swell[qp] = _swell_inc[qp];
  _total_swell[qp] += _total_swell_old[qp];

  Real p = -_trial_stress[qp].trace();
  Real pold = -_trial_stress_old[qp].trace();

  Real creep_inc_used=0.0;
  Real swell_inc_used=0.0;

  if (_integration_flag == 0 && _solve_definition == 1)
  {
    creep_inc_used = _DECRA[0];
    swell_inc_used = _DESWA[0];
  }
  else if (_integration_flag == 1 && _solve_definition == 2)
  {
    creep_inc_used = (_DECRA[1]*(_total_creep[qp]-_total_creep_old[qp]))+_creep_inc_old[qp];
    swell_inc_used = (_DESWA[1]*(_total_creep[qp]-_total_creep_old[qp]))+_swell_inc_old[qp];
  }
  else if (_integration_flag == 1 && _solve_definition == 3)
  {
    creep_inc_used = (_DECRA[2]*(_total_swell[qp]-_total_swell_old[qp]))+_creep_inc_old[qp];
    swell_inc_used = (_DESWA[2]*(_total_swell[qp]-_total_swell_old[qp]))+_swell_inc_old[qp];
  }
  else if (_integration_flag == 1 && _solve_definition == 4)
  {
    creep_inc_used = (_DECRA[3]*(p - pold))+_creep_inc_old[qp];
    swell_inc_used = (_DESWA[3]*(p - pold))+_swell_inc_old[qp];
  }
  else if (_integration_flag == 1 && _solve_definition == 5)
  {
    creep_inc_used = (_DECRA[4]*(_ets[qp]-_ets_old[qp]))+_creep_inc_old[qp];
    swell_inc_used = (_DESWA[4]*(_ets[qp]-_ets_old[qp]))+_swell_inc_old[qp];
  }

  // Calculate Incremental Creep Strain (total_effects)
  // Incremental creep strain = ((1/3)*(swell_inc_used)*R) + (creep_inc_used*grad_dts)
  // R = The matrix with the anisotropic swelling ratios in the diagonal if anisotropic swelling is defined; Otherwise R = Identity
  SymmTensor R(1.,1.,1.,0.,0.,0.);
  SymmTensor total_effects = (R*(swell_inc_used/3.)) + (grad_dts*(creep_inc_used));

  // Modify strain increment
  strain_increment += total_effects;

  // Update Stress
  SymmTensor stressnew(_stress_component[0], _stress_component[1], _stress_component[2],
                       _stress_component[3], _stress_component[4], _stress_component[5]);
  _stress[qp] = stressnew;
  _stress[qp] += _stress_old[qp];
}
