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
// Functions for the replicated fractional factorial sampling class 
// AUTHOR : CHARLES TONG
// DATE   : 2012
// ************************************************************************
#include <stdio.h>
#include <sstream>
using namespace std;

#include "Psuade.h"
#include "sysdef.h"
#include "PsuadeUtil.h"
#include "RFractFactSampling.h"

// ************************************************************************
// constructor 
// ------------------------------------------------------------------------
RFractFactSampling::RFractFactSampling() : Sampling()
{
   samplingID_ = PSUADE_SAMP_RFF4;
   resolution_ = 4;
}

// ************************************************************************
// destructor
// ------------------------------------------------------------------------
RFractFactSampling::~RFractFactSampling()
{
}

// ************************************************************************
// initialize the sampler data
// ------------------------------------------------------------------------
int RFractFactSampling::initialize(int initLevel)
{
   int    maxInputs, nInputCnt, ii, increment, count, nn;
   int    indices[12], ss, iii, nterms, kk, rr, nReps, offset, *iArray;
   double width;
   char   pString[500];

   if (nInputs_ == 0 || lowerBounds_ == NULL || upperBounds_ == NULL)
   {
      printf("RFractFactSampling::initialize ERROR - input not set up.\n");
      exit(1);
   }

   deleteSampleData();
   if (initLevel != 0) return 0;

   if (nInputs_ <= resolution_-1) nInputCnt = nInputs_;
   else
   {
      nInputCnt = resolution_ - 2;
      maxInputs = 0;
      while (maxInputs < nInputs_)
      {
         nInputCnt++;
         increment = resolution_ - 2;
         maxInputs = nInputCnt;
         while (nInputCnt-increment > 0)
         {
            increment++;
            count = 1;
            for (ii = nInputCnt; ii > nInputCnt-increment; ii--)
               count = count * ii / (nInputCnt - ii + 1); 
            maxInputs += count;
         }
      }
   }
   if (nInputCnt > 12)
   {
      printf("RFractFactSampling::initialize ERROR - nInputs > %d not",nInputs_);
      printf(" supported.\n");
      exit(1);
   }
   nSamples_ = (int) pow(2.0001, (double) nInputCnt);
   nReps = 1;
   if (psSamExpertMode_ == 1)
   {
      sprintf(pString, "How many replications ? (1 - 10000) : ");
      nReps = getInt(1, 10000, pString);
   }
   nSamples_ *= nReps;
   allocSampleData();

   if (printLevel_ > 3)
   {
      printf("RFractFactSampling::initialize: nSamples = %d\n", nSamples_);
      printf("RFractFactSampling::initialize: nInputs  = %d\n", nInputs_);
      printf("RFractFactSampling::initialize: nOutputs = %d\n", nOutputs_);
      if (printLevel_ > 4)
         for (ii = 0; ii < nInputs_; ii++)
            printf("    RFractFactSampling input %3d = [%e %e]\n", ii+1,
                   lowerBounds_[ii], upperBounds_[ii]);
   }

   iArray = new int[nInputs_];
   for (rr = 0; rr < nReps; rr++)
   {
      generateRandomIvector(nInputs_, iArray);
      offset = rr * nSamples_ / nReps;
      for (iii = 0; iii < nInputCnt; iii++)
      {
         ii = iArray[iii];
         increment = (int) pow(2.000001, iii+1);
         for (nn = 0; nn < nSamples_/nReps; nn+=increment)
         {
            for (ss = 0; ss < increment/2; ss++)
               sampleMatrix_[offset+nn+ss][ii] = -1;
            for (ss = 0; ss < increment/2; ss++)
               sampleMatrix_[offset+nn+increment/2+ss][ii] = 1;
         }
      }
      nterms = resolution_ - 1;
      for (ii = 0; ii < nterms; ii++) indices[ii] = ii;
      count = nInputCnt;
      while (count < nInputs_)
      {
         for (nn = 0; nn < nSamples_/nReps; nn++)
         {
            sampleMatrix_[offset+nn][iArray[count]] = 
               sampleMatrix_[offset+nn][iArray[indices[0]]] * 
               sampleMatrix_[offset+nn][iArray[indices[1]]] * 
               sampleMatrix_[offset+nn][iArray[indices[2]]]; 
            for (kk = 3; kk < nterms; kk++)
               sampleMatrix_[offset+nn][iArray[count]] *= 
                  sampleMatrix_[offset+nn][iArray[indices[kk]]]; 
         }
         kk = nterms - 1;
         while (kk >= 0 && indices[kk] >= nInputCnt+kk-nterms) kk--;
         if (kk < 0)
         {
            nterms++;
            for (ii = 0; ii < nterms; ii++) indices[ii] = ii;
         }
         else
         {
            indices[kk]++;
            for (ii = kk+1; ii < nterms; ii++) indices[ii] = indices[ii-1] + 1;
         }
         count++;
      }
   }
   
   for (rr = 0; rr < nReps; rr++)
   {
      offset = rr * nSamples_ / nReps;
      kk = PSUADE_rand() % 2;
      if (nReps == 1) kk = 0;
      for (nn = 0; nn < nSamples_/nReps; nn++)
      {
         for (ii = 0; ii < nInputs_; ii++) 
         {
            if (nReps == 1) width = (upperBounds_[ii] - lowerBounds_[ii]); 
            else            width = 0.75 * (upperBounds_[ii] - lowerBounds_[ii]);
            if (sampleMatrix_[offset+nn][ii] == -1)
            {
               if (kk == 0) sampleMatrix_[offset+nn][ii] = lowerBounds_[ii];
               else         sampleMatrix_[offset+nn][ii] = lowerBounds_[ii] + width/3.0;
            }
            else if (sampleMatrix_[offset+nn][ii] == 1)
            {
               if (kk == 0) sampleMatrix_[offset+nn][ii] = lowerBounds_[ii] + width;
               else         sampleMatrix_[offset+nn][ii] = upperBounds_[ii];
            }
         }
      }
   }
   delete [] iArray;
   return 0;
}

// ************************************************************************
// refine the sample space
// ------------------------------------------------------------------------
int RFractFactSampling::refine(int, int, double, int, double *)
{
   printf("RFractFactSampling::refine ERROR - not available.\n");
   exit(1);
   return 0;
}

// ************************************************************************
// set internal scheme
// ------------------------------------------------------------------------
int RFractFactSampling::setParam(string sparam)
{
   istringstream buffer;
   int           pos = sparam.find("setResolution");
   string        substr;
                                                                                
   if (pos >= 0)
   {
      substr = sparam.substr(14);
      buffer.str(substr);
      buffer >> resolution_;
      if (resolution_ != 4 && resolution_ != 5) resolution_ = 4;
      if (resolution_ == 4) samplingID_ = PSUADE_SAMP_FF4;
      else                  samplingID_ = PSUADE_SAMP_FF5;
   }
   return 0;
}

// ************************************************************************
// equal operator
// ------------------------------------------------------------------------
RFractFactSampling& RFractFactSampling::operator=(const RFractFactSampling &)
{
   printf("RFractFactSampling operator= ERROR: operation not allowed.\n");
   exit(1);
   return (*this);
}

