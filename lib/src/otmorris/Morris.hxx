//                                               -*- C++ -*-
/**
 *  @brief Morris
 *
 *  Copyright 2005-2016 Airbus-EDF-IMACS-Phimeca
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
 */
#ifndef OTMORRIS_MORRIS_HXX
#define OTMORRIS_MORRIS_HXX

#include <openturns/TypedInterfaceObject.hxx>
#include <openturns/StorageManager.hxx>
#include <openturns/NumericalPoint.hxx>
#include <openturns/NumericalSample.hxx>
#include "otmorris/OTMORRISprivate.hxx"

namespace OTMORRIS
{

class OTMORRIS_API Morris
  : public OT::PersistentObject
{
  CLASSNAME;

public:

  /** Default constructor */
  Morris();

  /** Standard constructor */
  Morris(OT::NumericalSample & inputSample, OT::NumericalSample & outputSample);

  /** Virtual constructor method */
  Morris * clone() const;

  // Get Mean/Standard deviation
  OT::NumericalPoint getMeanEffects() const;
  OT::NumericalPoint getStandardDeviationEffects() const;

  /** String converter */
  OT::String __repr__() const;

  /** Method save() stores the object through the StorageManager */
  virtual void save(OT::Advocate & adv) const;

  /** Method load() reloads the object from the StorageManager */
  virtual void load(OT::Advocate & adv);

private:
  OT::NumericalSample inputSample_;
  OT::NumericalSample outputSample_;
  // Elementary effects
  mutable OT::NumericalSample elementaryEffects_;
  OT::UnsignedInteger N_;

}; /* class Morris */

} /* namespace OTMORRIS */

#endif /* OTMORRIS_MORRIS_HXX */
