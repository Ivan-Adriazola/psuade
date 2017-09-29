// ************************************************************************
// Copyright (c) 2007   Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
// Written by the PSUADE team. 
// All rights reserved.
//
// Please see the COPYRIGHT and LICENSE file for the copyright notice,
// disclaimer, contact information and the GNU Lesser General Public License.
//
// PSUADE is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free 
// Software Foundation) version 2.1 dated February 1999.
//
// PSUADE is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the terms and conditions of the GNU Lesser
// General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
// ************************************************************************
// Definition for the class CorrelationAnalyzer
// AUTHOR : CHARLES TONG
// DATE   : 2004
// ************************************************************************

#ifndef __CORRELATIONANALYZERH__
#define __CORRELATIONANALYZERH__

#include "Analysis/Analyzer.h"

// ************************************************************************
// class definition
// ************************************************************************
                                                                                
class CorrelationAnalyzer : public Analyzer 
{

public:

   CorrelationAnalyzer();

   ~CorrelationAnalyzer();

   double analyze(aData &adata);

private:
   int computeMeanVariance(int nSamples, int xDim, double *X, double *xmean,
                           double *xvar, int xID, int flag);
   int computeCovariance(int nSamples,int nX,double *X, int nY, double *Y,
                         double *xmeans, double *xvars, double ymean,
                         double yvar, int yID, double *Rvalues);
};

#endif // __CORRELATIONANALYZERH__
