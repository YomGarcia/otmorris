//                                               -*- C++ -*-
/**
 *  @file  MorrisExperiment.cxx
 *  @brief MorrisExperiment
 *
 *  Copyright 2005-2015 Airbus-EDF-IMACS-Phimeca
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  @author: schueller
 */
#include "otmorris/MorrisExperiment.hxx"
#include <openturns/PersistentObjectFactory.hxx>
#include <openturns/KPermutationsDistribution.hxx>
#include <openturns/UserDefined.hxx>
#include <openturns/RandomGenerator.hxx>

using namespace OT;

namespace OTMORRIS
{

CLASSNAMEINIT(MorrisExperiment);

static Factory<MorrisExperiment> RegisteredFactory;


/** Constructor using a p-level grid  - Uniform(0,1)^d */
MorrisExperiment::MorrisExperiment(const Indices & levels, const UnsignedInteger N)
  : WeightedExperiment()
  , interval_(levels.getSize())
  , experiment_()
  , step_ (levels.getSize())
  , N_(N)
{
  // Compute step
  for (UnsignedInteger k = 0; k < levels.getSize(); ++k)
  {
    if (levels[k] <= 1)
      throw InvalidArgumentException(HERE) << "Levels should be at least 2; levels[" << k << "]=" << levels[k];
    step_[k] = 1.0 / (levels[k] - 1.0);
  }
}

/** Constructor using a p-level grid and intervals*/
MorrisExperiment::MorrisExperiment(const Indices & levels, const Interval & interval, const UnsignedInteger N)
  : WeightedExperiment()
  , interval_(interval)
  , experiment_()
  , step_ (levels.getSize())
  , N_(N)
{
  if (levels.getSize() != interval.getDimension())
    throw InvalidArgumentException(HERE) << "Levels and interval should be of same size. Here, level's size=" << levels.getSize()
                                         <<", interval's size=" << interval.getDimension();
  // Compute step
  for (UnsignedInteger k = 0; k < levels.getSize(); ++k)
  {
    if (levels[k] <= 1)
      throw InvalidArgumentException(HERE) << "Levels should be at least 2; levels[" << k << "]=" << levels[k];
    step_[k] = 1.0 / (levels[k] - 1.0);
  }
}

/** Constructor using NumericalSample, which is supposed to be an LHS design - Uniform(0,1)^d*/
MorrisExperiment::MorrisExperiment(const NumericalSample & lhsDesign, const UnsignedInteger N)
  : WeightedExperiment()
  , interval_(lhsDesign.getDimension())
  , experiment_(lhsDesign)
  , step_(NumericalPoint(lhsDesign.getDimension(), 0.5 / lhsDesign.getSize()))
  , N_(N)
{
  // Check that xMin & xMax are in [0,1]^d otherwise raise an exception
  const NumericalPoint xMin(lhsDesign.getMin());
  const NumericalPoint xMax(lhsDesign.getMax());
  for (UnsignedInteger k = 0; k < xMax.getSize(); ++k)
  {
    if ((xMin[k] < 0.0) || (xMax[k] > 1.0))
      throw InvalidArgumentException(HERE) << " Given design is not in [0,1]^d. xMin[" << k << "]=" <<  xMin[k]
                                           << ", xMax[" << k << "]=" <<  xMax[k];
  }
}

/** Constructor using NumericalSample, which is supposed to be an LHS design */
MorrisExperiment::MorrisExperiment(const NumericalSample & lhsDesign, const Interval & interval, const UnsignedInteger N)
  : WeightedExperiment()
  , interval_(interval)
  , experiment_()
  , step_(NumericalPoint(lhsDesign.getDimension(), 0.5 / lhsDesign.getSize()))
  , N_(N)

{
  if (lhsDesign.getDimension() != interval.getDimension())
    throw InvalidArgumentException(HERE) << "Levels and design should have same dimension. Here, design's dimension=" << lhsDesign.getDimension()
                                         <<", interval's size=" << interval.getDimension();
  // lhs should be defined in [0,1]^d
  const NumericalPoint lowerBound(interval_.getLowerBound());
  const NumericalPoint upperBound(interval_.getUpperBound());
  const NumericalPoint delta(upperBound - lowerBound);
  // Standard experiment
  experiment_ = lhsDesign - lowerBound;
  experiment_ /= delta;
}

/* Virtual constructor method */
MorrisExperiment * MorrisExperiment::clone() const
{
  return new MorrisExperiment(*this);
}

// Build the p-th column of the orientation matrix
NumericalPoint MorrisExperiment::getOrientationMatrixColumn(const UnsignedInteger p) const
{
  const UnsignedInteger dimension(step_.getDimension());
  if (p >= dimension)
    throw InvalidArgumentException(HERE) << "Could not build the column";
  NumericalPoint orientation(dimension + 1, 1.0);
  for (UnsignedInteger i = 0; i <= p; ++i) orientation[i] = -1.0;
  return orientation;
}

/** Generate method */
NumericalSample MorrisExperiment::generate() const
{
  const UnsignedInteger dimension(step_.getDimension());
  // Distribution that defines the permutations
  const KPermutationsDistribution permutationDistribution(dimension, dimension);
  // Distribution that defines the direction
  NumericalSample admissibleDirections(2, 1);
  admissibleDirections[0][0] = 1.0;
  admissibleDirections[1][0] = -1.0;
  const UserDefined directionDistribution(admissibleDirections);
  // Interval parameters
  const NumericalPoint lowerBound(interval_.getLowerBound());
  const NumericalPoint upperBound(interval_.getUpperBound());
  const NumericalPoint delta(upperBound - lowerBound);
  // Support sample for realizations
  NumericalSample realizations(N_ * (dimension + 1), dimension);

  for (UnsignedInteger k = 0; k < N_; ++k)
  {
    /* Generation of the k-th trajectory :
      1) Generation of an "xbase" point
      2) Generation of an orientation matrix B of size (dimension + 1) x dimension
      3) Generation of a permutation matrix P of size dimension x dimension
      4) Generation of a direction matrix D of size dimension x dimension
      5) Evaluate Z = (B * P * D + 1) * 0.5
      6) Compute Z * diag(step) + xbase
    */
    // First generate point
    NumericalPoint xBase;
    if (experiment_.getSize() > 0)
      xBase = generateXBaseFromLHS();
    else
      xBase = generateXBaseFromGrid();
    // Here we combine steps 2 to 6 as B * P permutes the columns of B
    // Define the permutations
    const NumericalPoint permutations(permutationDistribution.getRealization());
    // Define the direction
    const NumericalPoint directions(directionDistribution.getSample(dimension).getImplementation()->getData());

    for (UnsignedInteger p = 0; p < dimension; ++p)
    {
      // Steps  2 and 3  B * P ==> permutation of the orientation matrix
      const NumericalPoint orientationMatrixColumn(getOrientationMatrixColumn(p));

      // Steps 5 and 6
      for (UnsignedInteger i = 0; i < dimension + 1; ++i) realizations[k * (dimension + 1) + i][p] = delta[p] * ( (orientationMatrixColumn[i] * directions[p] + 1.0) * 0.5 * step_[p] + xBase[p] ) + lowerBound[p];
    }
  }
  // return sample
  return realizations;
}

// generate method with lhs design
NumericalPoint MorrisExperiment::generateXBaseFromLHS() const
{
  // Perform point from LHS
  const UnsignedInteger size(experiment_.getSize());
  const UnsignedInteger index(RandomGenerator::IntegerGenerate(size));
  return experiment_[index];
}

// generate method from a grid design
NumericalPoint MorrisExperiment::generateXBaseFromGrid() const
{
  // Generate points from regular grid
  const UnsignedInteger dimension(interval_.getDimension());
  NumericalPoint xBase(dimension, 0.0);
  for (UnsignedInteger p = 0; p < dimension; ++p)
  {
    const UnsignedInteger level(static_cast<UnsignedInteger>(1 + 1 /step_[p]));
    xBase[p] = step_[p] * RandomGenerator::IntegerGenerate(level - 1);
  }
  return xBase;
}

/* String converter */
String MorrisExperiment::__repr__() const
{
  OSS oss;
  oss << "class=" << MorrisExperiment::GetClassName();
  return oss;
}

/* Method save() stores the object through the StorageManager */
void MorrisExperiment::save(Advocate & adv) const
{
  WeightedExperiment::save( adv );
  adv.saveAttribute( "interval_", interval_ );
  adv.saveAttribute( "experiment_", experiment_ );
  adv.saveAttribute( "step_", step_ );
  adv.saveAttribute( "N_", N_ );
}

/* Method load() reloads the object from the StorageManager */
void MorrisExperiment::load(Advocate & adv)
{
  WeightedExperiment::load( adv );
  adv.loadAttribute( "interval_", interval_ );
  adv.loadAttribute( "experiment_", experiment_ );
  adv.loadAttribute( "step_", step_ );
  adv.loadAttribute( "N_", N_ );
}


} /* namespace OTMORRIS */
