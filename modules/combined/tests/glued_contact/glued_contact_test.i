[Mesh]
  file = glued_contact_test.e
  displacements = 'disp_x disp_y disp_z'
[]

[Functions]
  [./up]
    type = PiecewiseLinear
    x = '0 1'
    y = '0 0.5001'
  [../]

  [./lateral]
    type = PiecewiseLinear
    x = '0 1 2 3'
    y = '0 0 1 0'
    scale_factor = 0.5
  [../]
[]

[Variables]
  [./disp_x]
    order = FIRST
    family = LAGRANGE
  [../]

  [./disp_y]
    order = FIRST
    family = LAGRANGE
  [../]

  [./disp_z]
    order = FIRST
    family = LAGRANGE
  [../]
[] # Variables

[SolidMechanics]
  [./solid]
    disp_x = disp_x
    disp_y = disp_y
    disp_z = disp_z
  [../]
[]

[Contact]
  [./dummy_name]
    master = 2
    slave = 3
    disp_x = disp_x
    disp_y = disp_y
    disp_z = disp_z
    penalty = 1e6
    model = glued
    formulation = kinematic
  [../]
[]

[BCs]

  [./bottom_lateral]
    type = FunctionPresetBC
    variable = disp_x
    boundary = 1
    function = lateral
  [../]

  [./bottom_up]
    type = FunctionPresetBC
    variable = disp_y
    boundary = 1
    function = up
  [../]

  [./bottom_out]
    type = PresetBC
    variable = disp_z
    boundary = 1
    value = 0.0
  [../]

  [./top]
    type = PresetBC
    variable = disp_y
    boundary = 4
    value = 0.0
  [../]

[] # BCs

[Materials]

  [./stiffStuff1]
    type = Elastic
    block = 1

    disp_x = disp_x
    disp_y = disp_y
    disp_z = disp_z

    youngs_modulus = 1e6
    poissons_ratio = 0.3
  [../]

  [./stiffStuff2]
    type = Elastic
    block = 2

    disp_x = disp_x
    disp_y = disp_y
    disp_z = disp_z

    youngs_modulus = 1e6
    poissons_ratio = 0.3
  [../]
[] # Materials

[Executioner]
  type = Transient

  #Preconditioned JFNK (default)
  solve_type = 'PJFNK'



#  petsc_options_iname = '-pc_type -pc_hypre_type -snes_type -snes_ls -snes_linesearch_type -ksp_gmres_restart'
#  petsc_options_value = 'hypre    boomeramg      ls         basic    basic                    101'
  petsc_options_iname = '-pc_type -ksp_gmres_restart'
  petsc_options_value = 'ilu      101'


  line_search = 'none'


  nl_abs_tol = 1e-8
  nl_rel_tol = 1e-8
  l_tol = 1e-4

  l_max_its = 100
  nl_max_its = 10
  dt = 0.1
  num_steps = 30

  [./Predictor]
    type = SimplePredictor
    scale = 1.0
  [../]
[] # Executioner

[Postprocessors]
  active = ''
  [./resid]
    type = Residual
  [../]
  [./iters]
    type = NumNonlinearIterations
  [../]
[]

[Outputs]
  file_base = out
  output_initial = true
  exodus = true
  print_linear_residuals = true
  print_perf_log = true
[] # Outputs
