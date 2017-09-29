// ************************************************************************
// Copyright (c) 2007   Lawrence Livermore National Security, LLC. 
// Produced at the Lawrence Livermore National Laboratory.
// Written by the PSUADE team.
// All rights reserved.
//
// Please see the COPYRIGHT_and_LICENSE file for the copyright notice,
// disclaimer, contact information and the GNU Lesser General Public License.
//
// PSUADE is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License (as published by the Free Software
// Foundation) version 2.1 dated February 1999.
//
// PSUADE is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the terms and conditions of the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
// ************************************************************************
// Definition for the class PDFSpecial
// AUTHOR : CHARLES TONG
// DATE   : 2013
// ************************************************************************
#ifndef __PDFSPECIALH__
#define __PDFSPECIALH__

/**
 * @name User-provided distribution 
 *
 **/

// ************************************************************************
// class definition
// ************************************************************************
#include "PDFBase.h"

class PDFSpecial : public PDFBase
{
   int    nInputs_;
   int    nSamples_;
   double *samples_;

public:

   PDFSpecial(int);    
   ~PDFSpecial();

   int getPDF(int, double *, double *);
   int getCDF(int, double *, double *);
   int invCDF(int, double *, double *, double, double);
   int genSample(int, double *, double, double);
   double getMean();
};

#endif // __PDFSPECIALH__

