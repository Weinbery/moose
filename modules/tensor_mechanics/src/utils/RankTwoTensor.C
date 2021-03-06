#include "RankTwoTensor.h"

// Any other includes here
#include <vector>
#include <ostream>
#include "libmesh/libmesh.h"
#include "libmesh/tensor_value.h"
#include "MaterialProperty.h"

template<>
void mooseSetToZero<RankTwoTensor>(RankTwoTensor & v)
{
  v.zero();
}

RankTwoTensor::RankTwoTensor()
{
  mooseAssert(N == 3, "RankTwoTensor is currently only tested for 3 dimensions.");

  for (unsigned int i(0); i<N; i++)
    for (unsigned int j(0); j<N; j++)
      _vals[i][j] = 0.0;
}

RankTwoTensor::RankTwoTensor(const InitMethod init)
{
  switch (init)
  {
    case initNone:
      break;

    case initIdentity:
      for (unsigned int i = 0; i < N; ++i)
        for (unsigned int j = 0; j < N; ++j)
          _vals[i][j] = (i==j);
      break;

    default:
      mooseError("Unknown RankTwoTensor initialization pattern.");
  }
}

RankTwoTensor::RankTwoTensor(const TypeVector<Real> & row1, const TypeVector<Real> & row2, const TypeVector<Real> & row3)
{
  // Initialize the Tensor matrix from the passed in vectors
  for (unsigned int i=0; i<N; i++)
    _vals[0][i] = row1(i);

  for (unsigned int i=0; i<N; i++)
    _vals[1][i] = row2(i);

  for (unsigned int i=0; i<N; i++)
    _vals[2][i] = row3(i);
}

RankTwoTensor::RankTwoTensor(const RankTwoTensor & a)
{
  *this = a;
}

RankTwoTensor::RankTwoTensor(const TypeTensor<Real> & a)
{
  for (unsigned int i(0); i<N; i++)
    for (unsigned int j(0); j<N; j++)
      _vals[i][j] = a(i,j);
}

RankTwoTensor::RankTwoTensor(Real S11, Real S22, Real S33, Real S23, Real S13, Real S12)
{
  _vals[0][0] = S11;
  _vals[1][1] = S22;
  _vals[2][2] = S33;
  _vals[1][2] = _vals[2][1] = S23;
  _vals[0][2] = _vals[2][0] = S13;
  _vals[0][1] = _vals[1][0] = S12;
}

RankTwoTensor::RankTwoTensor(Real S11, Real S21, Real S31, Real S12, Real S22, Real S32, Real S13, Real S23, Real S33)
{
  _vals[0][0] = S11;
  _vals[1][0] = S21;
  _vals[2][0] = S31;
  _vals[0][1] = S12;
  _vals[1][1] = S22;
  _vals[2][1] = S32;
  _vals[0][2] = S13;
  _vals[1][2] = S23;
  _vals[2][2] = S33;
}

Real &
RankTwoTensor::operator()(unsigned int i, unsigned int j)
{
  return _vals[i][j];
}

Real
RankTwoTensor::operator()(unsigned int i, unsigned int j) const
{
  return _vals[i][j];
}


void
RankTwoTensor::zero()
{
  for (unsigned int i(0); i<N; i++)
    for (unsigned int j(0); j<N; j++)
      _vals[i][j] = 0.0;
}

void
RankTwoTensor::fillFromInputVector(const std::vector<Real> & input)
{
  if (input.size() == 6)
  {
    // Moose::out << "Rank 2 tensor input size =" << input.size() << std::endl;
    _vals[0][0] = input[0]; //S11
    _vals[1][1] = input[1]; //S22
    _vals[2][2] = input[2]; //S33
    _vals[1][2] = _vals[2][1] = input[3]; //S23
    _vals[0][2] = _vals[2][0] = input[4]; //S13
    _vals[0][1] = _vals[1][0] = input[5]; //S12
  }
  else if (input.size() == 9)
  {
    // Moose::out << "Rank 2 tensor input size =" << input.size() << std::endl;
    _vals[0][0] = input[0]; //S11
    _vals[1][0] = input[1]; //S21
    _vals[2][0] = input[2]; //S31
    _vals[0][1] = input[3]; //S12
    _vals[1][1] = input[4]; //S22
    _vals[2][1] = input[5]; //S32
    _vals[0][2] = input[6]; //S13
    _vals[1][2] = input[7]; //S23
    _vals[2][2] = input[8]; //S33
  }
  else
    mooseError("Please check the number of entries in the eigenstrain input vector.  It must be 6 or 9");
}

TypeVector<Real>
RankTwoTensor::row(const unsigned int r) const
{
  RealVectorValue result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; i++)
    result(i) = a(r,i);

  return result;
}

void
RankTwoTensor::rotate(RealTensorValue & R)
{
  RankTwoTensor temp;
  for (unsigned int i = 0; i < N; i++)
    for (unsigned int j = 0; j < N; j++)
    {
      for (unsigned int k = 0; k < N; k++)
        for (unsigned int l = 0; l < N; l++)
          temp(i, j) += R(i,k)*R(j,l)*_vals[k][l];
    }
  for (unsigned int i = 0; i < N; i++)
    for (unsigned int j = 0; j < N; j++)
      _vals[i][j] = temp(i, j);
}

void
RankTwoTensor::rotate(RankTwoTensor & R)
{
  RankTwoTensor temp;
  for (unsigned int i = 0; i < N; i++)
    for (unsigned int j = 0; j < N; j++)
    {
      for (unsigned int k = 0; k < N; k++)
        for (unsigned int l = 0; l < N; l++)
          temp(i, j) += R(i,k) * R(j,l) * _vals[k][l];
    }
  for (unsigned int i = 0; i < N; i++)
    for (unsigned int j = 0; j < N; j++)
      _vals[i][j] = temp(i, j);
}

RankTwoTensor
RankTwoTensor::rotateXyPlane(Real a)
{
  Real c = std::cos(a);
  Real s = std::sin(a);
  Real x = _vals[0][0]*c*c + _vals[1][1]*s*s + 2.0*_vals[0][1]*c*s;
  Real y = _vals[0][0]*s*s + _vals[1][1]*c*c - 2.0*_vals[0][1]*c*s;
  Real xy = (_vals[1][1] - _vals[0][0])*c*s + _vals[0][1]*(c*c - s*s);

  RankTwoTensor b(*this);

  b(0,0) = x;
  b(1,1) = y;
  b(1,0) = b(0,1) = xy;

  return b;
}

RankTwoTensor
RankTwoTensor::transpose() const
{
  RankTwoTensor result;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      result(i,j) = _vals[j][i];

  return result;
}

RankTwoTensor &
RankTwoTensor::operator= (const RankTwoTensor & a)
{
  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      _vals[i][j] = a(i,j);

  return *this;
}

RankTwoTensor &
RankTwoTensor::operator+=(const RankTwoTensor & a)
{
  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      _vals[i][j] += a(i,j);

  return *this;
}

RankTwoTensor
RankTwoTensor::operator+ (const RankTwoTensor & b) const
{
  RankTwoTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      result(i,j) = a(i,j) + b(i,j);

  return result;
}

RankTwoTensor &
RankTwoTensor::operator-=(const RankTwoTensor & a)
{
  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      _vals[i][j] -= a(i,j);

  return *this;
}

RankTwoTensor
RankTwoTensor::operator- (const RankTwoTensor & b) const
{
  RankTwoTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      result(i,j) = a(i,j) - b(i,j);

  return result;
}

RankTwoTensor
RankTwoTensor::operator - () const
{
  RankTwoTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      result(i,j) = -a(i,j);

  return result;
}

RankTwoTensor &
RankTwoTensor::operator*=(const Real a)
{
  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      _vals[i][j] *= a;

  return *this;
}

RankTwoTensor
RankTwoTensor::operator*(const Real b) const
{
  RankTwoTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      result(i,j) = a(i,j) * b;

  return result;
}

RankTwoTensor &
RankTwoTensor::operator/=(const Real a)
{
  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      _vals[i][j] /= a;

  return *this;
}

RankTwoTensor
RankTwoTensor::operator/(const Real b) const
{
  RankTwoTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      result(i,j) = a(i,j) / b;

  return result;
}

RankTwoTensor &
RankTwoTensor::operator*=(const RankTwoTensor & a)
{
  RankTwoTensor s(*this);
  this->zero();

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      for (unsigned int k = 0; k < N; ++k)
        _vals[i][k] += s(i,j) * a(j,k);

  return *this;
}

RankTwoTensor
RankTwoTensor::operator*(const RankTwoTensor & b) const
{
  RankTwoTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      for (unsigned int k = 0; k < N; ++k)
        result(i,k) += a(i,j) * b(j,k);
  return result;
}

RankTwoTensor
RankTwoTensor::operator*(const TypeTensor<Real> & b) const
{
  RankTwoTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      for (unsigned int k = 0; k < N; ++k)
        result(i,k) += a(i,j) * b(j,k);

  return result;
}

Real
RankTwoTensor::doubleContraction(const RankTwoTensor & b) const
{
  Real result = 0.0;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      result += a(i,j) * b(i,j);

  return result;
}


RankFourTensor
RankTwoTensor::outerProduct(const RankTwoTensor & b) const
{
  RankFourTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      for (unsigned int k = 0; k < N; ++k)
        for (unsigned int l = 0; l < N; ++l)
          result(i,j,k,l) = a(i,j) * b(k,l);

  return result;
}

RankFourTensor
RankTwoTensor::mixedProductIkJl(const RankTwoTensor & b) const
{
  RankFourTensor result;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      for (unsigned int k = 0; k < N; ++k)
        for (unsigned int l = 0; l < N; ++l)
          result(i,j,k,l) = a(i,k) * b(j,l);

  return result;
}

RankTwoTensor
RankTwoTensor::deviatoric() const
{
  RankTwoTensor deviatoric(*this);
  deviatoric.addIa(-1.0/3.0 * trace()); // actually construct deviatoric part
  return deviatoric;
}

Real
RankTwoTensor::secondInvariant() const
{
  Real result = 0.0;
  const RankTwoTensor &a = *this;

  //RankTwoTensor deviatoric(*this);
  //deviatoric.addIa(-1.0/3.0 * trace()); // actually construct deviatoric part
  //result = 0.5*(deviatoric + deviatoric.transpose()).doubleContraction(deviatoric + deviatoric.transpose());
  result = std::pow(a(0,0) - a(1,1), 2)/6.0;
  result += std::pow(a(0,0) - a(2,2), 2)/6.0;
  result += std::pow(a(1,1) - a(2,2), 2)/6.0;
  result += std::pow(a(0,1) + a(1,0), 2)/4.0;
  result += std::pow(a(0,2) + a(2,0), 2)/4.0;
  result += std::pow(a(1,2) + a(2,1), 2)/4.0;
  return result;
}

RankTwoTensor
RankTwoTensor::dsecondInvariant() const
{
  return 0.5 * (deviatoric() + deviatoric().transpose());
}

RankFourTensor
RankTwoTensor::d2secondInvariant() const
{
  RankFourTensor result;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      for (unsigned int k = 0; k < N; ++k)
        for (unsigned int l = 0; l < N; ++l)
          result(i,j,k,l) = 0.5*(i==k)*(j==l) + 0.5*(i==l)*(j==k) - (1.0/3.0)*(i==j)*(k==l);

  return result;
}

Real
RankTwoTensor::trace() const
{
  Real result = 0.0;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
      result += a(i,i);

  return result;
}

RankTwoTensor
RankTwoTensor::dtrace() const
{
  return RankTwoTensor(1, 0, 0, 0, 1, 0, 0, 0, 1);
}


Real
RankTwoTensor::thirdInvariant() const
{
  RankTwoTensor s = 0.5 * deviatoric();
  s += s.transpose();

  Real result = 0.0;

  result =  s(0, 0) * (s(1, 1) * s(2, 2) - s(2, 1) * s(1, 2));
  result -= s(1, 0) * (s(0, 1) * s(2, 2) - s(2, 1) * s(0, 2));
  result += s(2, 0) * (s(0, 1) * s(1, 2) - s(1, 1) * s(0, 2));

  return result;
}

RankTwoTensor
RankTwoTensor::dthirdInvariant() const
{
  RankTwoTensor s = 0.5 * deviatoric();
  s += s.transpose();

  RankTwoTensor d;
  Real sec_over_three = secondInvariant() / 3.0;

  d(0, 0) = s(1, 1) * s(2, 2) - s(2, 1) * s(1, 2) + sec_over_three;
  d(0, 1) = s(2, 0) * s(1, 2) - s(1, 0) * s(2, 2);
  d(0, 2) = s(1, 0) * s(2, 1) - s(2, 0) * s(1, 1);
  d(1, 0) = s(2, 1) * s(0, 2) - s(0, 1) * s(2, 2);
  d(1, 1) = s(0, 0) * s(2, 2) - s(2, 0) * s(0, 2) + sec_over_three;
  d(1, 2) = s(2, 0) * s(0, 1) - s(0, 0) * s(2, 1);
  d(2, 0) = s(0, 1) * s(1, 2) - s(1, 1) * s(0, 2);
  d(2, 1) = s(1, 0) * s(0, 2) - s(0, 0) * s(1, 2);
  d(2, 2) = s(0, 0) * s(1, 1) - s(1, 0) * s(0, 1) + sec_over_three;

  return d;
}

RankFourTensor
RankTwoTensor::d2thirdInvariant() const
{
  RankTwoTensor s = 0.5 * deviatoric();
  s += s.transpose();

  RankFourTensor d2;
  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      for (unsigned int k = 0; k < N; ++k)
        for (unsigned int l = 0; l < N; ++l)
        {
          d2(i, j, k, l) = (i==j)*s(k, l)/3.0 + (k==l)*s(i, j)/3.0;
          //for (unsigned int a = 0; a < N; ++a)
          //  for (unsigned int b = 0; b < N; ++b)
          //    d2(i, j, k, l) += 0.5*(PermutationTensor::eps(i, k, a)*PermutationTensor::eps(j, l, b) + PermutationTensor::eps(i, l, a)*PermutationTensor::eps(j, k, b))*s(a, b);
        }

  // I'm not sure which is more readable: the above
  // PermutationTensor stuff, or the stuff below.
  // Anyway, they yield the same result, and so i leave
  // both of them here to enlighten you!

  d2(0, 0, 1, 1) += s(2, 2);
  d2(0, 0, 1, 2) -= s(2, 1);
  d2(0, 0, 2, 1) -= s(1, 2);
  d2(0, 0, 2, 2) += s(1, 1);

  d2(0, 1, 0, 1) -= s(2, 2)/2.0;
  d2(0, 1, 1, 0) -= s(2, 2)/2.0;
  d2(0, 1, 0, 2) += s(1, 2)/2.0;
  d2(0, 1, 2, 0) += s(1, 2)/2.0;
  d2(0, 1, 1, 2) += s(2, 0)/2.0;
  d2(0, 1, 2, 1) += s(2, 0)/2.0;
  d2(0, 1, 2, 2) -= s(1, 0);

  d2(0, 2, 0, 1) += s(2, 1)/2.0;
  d2(0, 2, 1, 0) += s(2, 1)/2.0;
  d2(0, 2, 0, 2) -= s(1, 1)/2.0;
  d2(0, 2, 2, 0) -= s(1, 1)/2.0;
  d2(0, 2, 1, 1) -= s(2, 0);
  d2(0, 2, 1, 2) += s(1, 0)/2.0;
  d2(0, 2, 2, 1) += s(1, 0)/2.0;

  d2(1, 0, 0, 1) -= s(2, 2)/2.0;
  d2(1, 0, 1, 0) -= s(2, 2)/2.0;
  d2(1, 0, 0, 2) += s(1, 2)/2.0;
  d2(1, 0, 2, 0) += s(1, 2)/2.0;
  d2(1, 0, 1, 2) += s(2, 0)/2.0;
  d2(1, 0, 2, 1) += s(2, 0)/2.0;
  d2(1, 0, 2, 2) -= s(1, 0);

  d2(1, 1, 0, 0) += s(2, 2);
  d2(1, 1, 0, 2) -= s(2, 0);
  d2(1, 1, 2, 0) -= s(2, 0);
  d2(1, 1, 2, 2) += s(0, 0);

  d2(1, 2, 0, 0) -= s(2, 1);
  d2(1, 2, 0, 1) += s(2, 0)/2.0;
  d2(1, 2, 1, 0) += s(2, 0)/2.0;
  d2(1, 2, 0, 2) += s(0, 1)/2.0;
  d2(1, 2, 2, 0) += s(0, 1)/2.0;
  d2(1, 2, 1, 2) -= s(0, 0)/2.0;
  d2(1, 2, 2, 1) -= s(0, 0)/2.0;

  d2(2, 0, 0, 1) += s(2, 1)/2.0;
  d2(2, 0, 1, 0) += s(2, 1)/2.0;
  d2(2, 0, 0, 2) -= s(1, 1)/2.0;
  d2(2, 0, 2, 0) -= s(1, 1)/2.0;
  d2(2, 0, 1, 1) -= s(2, 0);
  d2(2, 0, 1, 2) += s(1, 0)/2.0;
  d2(2, 0, 2, 1) += s(1, 0)/2.0;

  d2(2, 1, 0, 0) -= s(2, 1);
  d2(2, 1, 0, 1) += s(2, 0)/2.0;
  d2(2, 1, 1, 0) += s(2, 0)/2.0;
  d2(2, 1, 0, 2) += s(0, 1)/2.0;
  d2(2, 1, 2, 0) += s(0, 1)/2.0;
  d2(2, 1, 1, 2) -= s(0, 0)/2.0;
  d2(2, 1, 2, 1) -= s(0, 0)/2.0;

  d2(2, 2, 0, 0) += s(1, 1);
  d2(2, 2, 0, 1) -= s(1, 0);
  d2(2, 2, 1, 0) -= s(1, 0);
  d2(2, 2, 1, 1) += s(0, 0);

  return d2;
}

Real
RankTwoTensor::sin3Lode(const Real r0, const Real r0_value) const
{
  Real bar = secondInvariant();
  if (bar <= r0)
    // in this case the Lode angle is not defined
    return r0_value;
  else
    // the min and max here gaurd against precision-loss when bar is tiny but nonzero.
    return std::max(std::min(-1.5 * std::sqrt(3.0) * thirdInvariant() / std::pow(bar, 1.5), 1.0), -1.0);
}

RankTwoTensor
RankTwoTensor::dsin3Lode(const Real r0) const
{
  Real bar = secondInvariant();
  if (bar <= r0)
    return RankTwoTensor();
  else
    return -1.5 * std::sqrt(3.0) * (dthirdInvariant() / std::pow(bar, 1.5) - 1.5 * dsecondInvariant() * thirdInvariant() / std::pow(bar, 2.5));
}

RankFourTensor
RankTwoTensor::d2sin3Lode(const Real r0) const
{
  Real bar = secondInvariant();
  if (bar <= r0)
    return RankFourTensor();

  Real J3 = thirdInvariant();
  RankTwoTensor dII = dsecondInvariant();
  RankTwoTensor dIII = dthirdInvariant();
  RankFourTensor deriv = d2thirdInvariant()/std::pow(bar, 1.5) - 1.5*d2secondInvariant()*J3/std::pow(bar, 2.5);

  for (unsigned i = 0 ; i < N ; ++i)
    for (unsigned j = 0 ; j < N ; ++j)
      for (unsigned k = 0 ; k < N ; ++k)
        for (unsigned l = 0 ; l < N ; ++l)
          deriv(i, j, k, l) += (-1.5*dII(i, j)*dIII(k, l) -1.5 * dIII(i, j) * dII(k, l)) / std::pow(bar, 2.5) + 1.5*2.5*dII(i, j) * dII(k, l) * J3 / std::pow(bar, 3.5);

  deriv *= -1.5 * std::sqrt(3.0);
  return deriv;
}

Real
RankTwoTensor::det() const
{
  Real result = 0.0;
  const RankTwoTensor &a = *this;

  result =  a(0,0) * (a(1,1) * a(2,2) - a(2,1) * a(1,2));
  result -= a(1,0) * (a(0,1) * a(2,2) - a(2,1) * a(0,2));
  result += a(2,0) * (a(0,1) * a(1,2) - a(1,1) * a(0,2));

  return result;
}


RankTwoTensor
RankTwoTensor::ddet() const
{
  RankTwoTensor d;
  const RankTwoTensor &a = *this;

  d(0, 0) = a(1,1) * a(2,2) - a(2,1) * a(1,2);
  d(0, 1) = a(2,0) * a(1,2) - a(1,0) * a(2,2);
  d(0, 2) = a(1,0) * a(2,1) - a(2,0) * a(1,1);
  d(1, 0) = a(2,1) * a(0,2) - a(0,1) * a(2,2);
  d(1, 1) = a(0,0) * a(2,2) - a(2,0) * a(0,2);
  d(1, 2) = a(2,0) * a(0,1) - a(0,0) * a(2,1);
  d(2, 0) = a(0,1) * a(1,2) - a(1,1) * a(0,2);
  d(2, 1) = a(1,0) * a(0,2) - a(0,0) * a(1,2);
  d(2, 2) = a(0,0) * a(1,1) - a(1,0) * a(0,1);

  return d;
}

RankTwoTensor
RankTwoTensor::inverse() const
{
  RankTwoTensor result;
  const RankTwoTensor &a = *this;

  result(0,0) = a(1,1) * a(2,2) - a(2,1) * a(1,2);
  result(0,1) = a(0,2) * a(2,1) - a(0,1) * a(2,2);
  result(0,2) = a(0,1) * a(1,2) - a(0,2) * a(1,1);
  result(1,0) = a(1,2) * a(2,0) - a(1,0) * a(2,2);
  result(1,1) = a(0,0) * a(2,2) - a(0,2) * a(2,0);
  result(1,2) = a(0,2) * a(1,0) - a(0,0) * a(1,2);
  result(2,0) = a(1,0) * a(2,1) - a(1,1) * a(2,0);
  result(2,1) = a(0,1) * a(2,0) - a(0,0) * a(2,1);
  result(2,2) = a(0,0) * a(1,1) - a(0,1) * a(1,0);

  Real det = (*this).det();

  if (det == 0)
    mooseError("Rank Two Tensor is singular");

  result /= det;
  return result;
}

void
RankTwoTensor::print() const
{
  const RankTwoTensor &a = *this;
  for (unsigned int i = 0; i < N; ++i)
  {
    for (unsigned int j = 0; j < N; ++j)
      Moose::out << std::setw(15) << a(i,j) << ' ';
    Moose::out <<std::endl;
  }
}

void
RankTwoTensor::addIa(const Real a)
{
  for (unsigned int i = 0; i < N; ++i)
    _vals[i][i] += a;
}

Real
RankTwoTensor::L2norm() const
{
  Real norm = 0.0;
  const RankTwoTensor &a = *this;

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      norm += a(i,j) * a(i,j);

  norm = std::sqrt(norm);
  return norm;
}

void
RankTwoTensor::surfaceFillFromInputVector(const std::vector<Real> & input)
{
  if (input.size() == 4)
  {
    // initialize with zeros
    this->zero();
    _vals[0][0] = input[0];
    _vals[0][1] = input[1];
    _vals[1][0] = input[2];
    _vals[1][1] = input[3];
  }
  else
    mooseError("please provide correct number of values for surface RankTwoTensor initialization.");
}

void
RankTwoTensor::symmetricEigenvalues(std::vector<Real> & eigvals) const
{
  std::vector<PetscScalar> a;
  syev("N", eigvals, a);
}

void
RankTwoTensor::dsymmetricEigenvalues(std::vector<Real> & eigvals, std::vector<RankTwoTensor> & deigvals) const
{
  deigvals.resize(N);

  std::vector<PetscScalar> a;
  syev("V", eigvals, a);

  // now a contains the eigenvetors
  // extract these and place appropriately in deigvals
  std::vector<Real> eig_vec;
  eig_vec.resize(N);

  for (unsigned int i = 0; i < N; ++i)
  {
    for (unsigned int j = 0; j < N; ++j)
      eig_vec[j] = a[i*N + j];
    for (unsigned int j = 0; j < N; ++j)
      for (unsigned int k = 0; k < N; ++k)
        deigvals[i](j, k) = eig_vec[j] * eig_vec[k];
  }

  // There are discontinuities in the derivative
  // for equal eigenvalues.  The following is
  // an attempt to make a sensible choice for
  // the derivative.  This agrees with a central-difference
  // approximation to the derivative.
  if (eigvals[0] == eigvals[1] && eigvals[0] == eigvals[2])
    deigvals[0] = deigvals[1] = deigvals[2] = (deigvals[0] + deigvals[1] + deigvals[2]) / 3.0;
  else if (eigvals[0] == eigvals[1])
    deigvals[0] = deigvals[1] = (deigvals[0] + deigvals[1]) / 2.0;
  else if (eigvals[0] == eigvals[2])
    deigvals[0] = deigvals[2] = (deigvals[0] + deigvals[2]) / 2.0;
  else if (eigvals[1] == eigvals[2])
    deigvals[1] = deigvals[2] = (deigvals[1] + deigvals[2]) / 2.0;
}

void
RankTwoTensor::d2symmetricEigenvalues(std::vector<RankFourTensor> & deriv) const
{
  std::vector<PetscScalar> eigvec;
  std::vector<PetscScalar> eigvals;
  Real ev[N][N];

  // reset rank four tensor
  deriv.assign(N, RankFourTensor());

  // get eigen values and eigen vectors
  syev("V", eigvals, eigvec);

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      ev[i][j] = eigvec[i*N + j];

  for (unsigned int alpha = 0; alpha < N; ++alpha)
    for (unsigned int beta = 0; beta < N; ++beta)
    {
      if (eigvals[alpha] == eigvals[beta])
        continue;

      for (unsigned int i = 0; i < N; ++i)
        for (unsigned int j = 0; j < N; ++j)
          for (unsigned int k = 0; k < N; ++k)
            for (unsigned int l = 0; l < N; ++l)
            {
              deriv[alpha](i, j, k, l) += 0.5 * (ev[beta][i] * ev[alpha][j] + ev[alpha][i] * ev[beta][j])
                                              * (ev[beta][k] * ev[alpha][l] + ev[beta][l] * ev[alpha][k])
                                              / (eigvals[alpha] - eigvals[beta]);
            }
    }
}

void
RankTwoTensor::syev(const char * calculation_type, std::vector<PetscScalar> & eigvals, std::vector<PetscScalar> & a) const
{
  eigvals.resize(N);
  a.resize(N*N);

  // prepare data for the LAPACKsyev_ routine (which comes from petscblaslapack.h)
  int nd = N;
  int lwork = 66 * nd;
  int info;
  std::vector<PetscScalar> work(lwork);

  for (unsigned int i = 0; i < N; ++i)
    for (unsigned int j = 0; j < N; ++j)
      // a is destroyed by dsyev, and if calculation_type == "V" then eigenvectors are placed there
      // Note the explicit symmeterisation
      a[i*N + j] = 0.5 * (this->operator()(i,j) + this->operator()(j,i));

  // compute the eigenvalues only (if calculation_type == "N"),
  // or both the eigenvalues and eigenvectors (if calculation_type == "V")
  // assume upper triangle of a is stored (second "U")
  LAPACKsyev_(calculation_type, "U", &nd, &a[0], &nd, &eigvals[0], &work[0], &lwork, &info);

  if (info != 0)
    mooseError("In computing the eigenvalues and eigenvectors of a symmetric rank-2 tensor, the PETSC LAPACK syev routine returned error code " << info);
}
