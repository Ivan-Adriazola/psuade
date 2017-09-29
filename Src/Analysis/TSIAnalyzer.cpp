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
// Functions for the TSIAnalyzer class 
// AUTHOR : CHARLES TONG
// DATE   : 2020
// ************************************************************************
#include <stdio.h>
#include <assert.h>
#include <sstream>
using namespace std;

#include "Psuade.h"
#include "TSIAnalyzer.h"
#include "sysdef.h"
#include "PsuadeUtil.h"

#ifdef HAVE_METIS
extern "C" 
{
void METIS_PartGraphRecursive(int *, int *, int *, int *, int *,
                              int *, int *, int *, int *, int *, int *);
}
#endif

// ************************************************************************
// constructor 
// ------------------------------------------------------------------------
TSIAnalyzer::TSIAnalyzer() : Analyzer()
{
}

// ************************************************************************
// destructor
// ------------------------------------------------------------------------
TSIAnalyzer::~TSIAnalyzer()
{
}

// ************************************************************************
// initialize the sampler data
// ------------------------------------------------------------------------
double TSIAnalyzer::analyze(aData &adata)
{
   int    ss, ii, jj, nInputs, nOutputs, nSamples, outputID, inputID, nnz;
   int    *incrs, itmp, jtmp, *sample2Aggr, *cellsOccupied, n1d, nAggrs;
   int    options[10], *aggrCnts, status, wFlag;
#ifdef HAVE_METIS
   int    wgtflag=0, numflag=0, edgeCut=0;
#endif
   int    graphN, *graphI, *graphJ, printLevel, index, totalCnt;
   double *lbounds, *ubounds, *ranges, *X, *Y, *YY, *aggrMean, dvar, dmean;
   double variance, *tsi, dtmp;
   char   pString[500];
   PsuadeData *ioPtr;

   printLevel = adata.printLevel_;
   nInputs  = adata.nInputs_;
   nOutputs = adata.nOutputs_;
   nSamples = adata.nSamples_;
   X        = adata.sampleInputs_;
   YY       = adata.sampleOutputs_;
   outputID = adata.outputID_;
   lbounds  = adata.iLowerB_;
   ubounds  = adata.iUpperB_;
   ioPtr    = adata.ioPtr_;
   if (ioPtr == NULL)
   {
      printf("TSIAnalyzer ERROR: no data.\n");
      return PSUADE_UNDEFINED;
   }

   if (nInputs <= 0 || nOutputs <= 0)
   {
      printf("Total Effect ERROR: invalid nInputs or nOutputs.\n");
      printf("   nInputs  = %d\n", nInputs);
      printf("   nOutputs = %d\n", nOutputs);
      return -1;
   }  
   if (nSamples <= 1)
   {
      printf("Total Effect ERROR: nSamples should be > 1.\n");
      printf("   nSamples = %d\n", nSamples);
      return -1;
   }  
   if (nSamples < 10000)
   {
      printf("Total Effect WARNING: nSamples may be too small to\n");
      printf("             give results with acceptable accuracy.\n");
   }  
   status = 0;
   for (ss = 0; ss < nSamples; ss++)
      if (YY[nOutputs*ss+outputID] > 0.9*PSUADE_UNDEFINED) status = 1;
   if (status == 1)
   {
      printf("Total Effect ERROR: Some outputs are undefined. Prune the\n");
      printf("                    undefined sample points first.\n");
      return PSUADE_UNDEFINED;
   }
   Y = new double[nSamples];
   for (ss = 0; ss < nSamples; ss++) Y[ss] = YY[ss*nOutputs+outputID];

   ranges  = new double[nInputs];
   for (ii = 0; ii < nInputs; ii++)
   {
      ranges[ii] = ubounds[ii] - lbounds[ii];
      if (ranges[ii] <= 0.0)
      {
         printf("Total Effect ERROR: lbound/ubound mismatch.\n");
         exit(1);
      }
   }

   dmean = 0.0;
   for (ss = 0; ss < nSamples; ss++) dmean += Y[ss];
   dmean /= (double) nSamples;
   variance = 0.0;
   for (ss = 0; ss < nSamples; ss++)
   {
      variance += ((Y[ss] - dmean) * (Y[ss] - dmean));
   }
   variance /= (double) (nSamples - 1);
   printf("Total Effect: output mean     = %e\n", dmean);
   printf("Total Effect: output variance = %e\n", variance);

   if (nInputs > 21)
   {
      printf("Total Effect ERROR: nInputs > 21 currently not supported.\n");
      exit(1);
   }
   if (nInputs == 1 ) n1d = nSamples*10;
   if (nInputs == 2 ) n1d = 1024;
   if (nInputs == 3 ) n1d = 100;
   if (nInputs == 4 ) n1d = 36;
   if (nInputs == 5 ) n1d = 16;
   if (nInputs == 6 ) n1d = 11;
   if (nInputs == 7 ) n1d = 8;
   if (nInputs == 8 ) n1d = 6;
   if (nInputs == 9 ) n1d = 5;
   if (nInputs == 10) n1d = 4;
   if (nInputs == 11) n1d = 3;
   if (nInputs == 12) n1d = 3;
   if (nInputs == 13) n1d = 3;
   if (nInputs >= 14) n1d = 2;

   printAsterisks(0);
   printf("*          Crude Total Sensitivity Indices\n");
   printEquals(0);
   printf("* Total Effect: number of subdomains          = %d\n",
          nSamples/50);
   printf("* Total Effect: number of point per subdomain = 50\n");
   printDashes(0);
   printf("* This may need to be adjusted for higher accuracy.\n");
   printf("* Recommendation: Try different numbers of subdomains.\n");
   printf("*   Generally, each subdomain should have at least\n");
   printf("*   50 sample points.\n");
   printf("* Turn on analysis expert mode to modify settings.\n");
   if (psAnaExpertMode_ != 0)
   {
      strcpy(pString, "Enter the number of subdomains (> 5): ");
      nAggrs = getInt(5, nSamples, pString);
   }
   else nAggrs = nSamples / 50;
   if (printLevel > 1)
      printf("* Total Effect: number of subdomains = %d\n", nAggrs);
   printEquals(0);

   incrs  = new int[nInputs];
   graphN = 1;
   incrs[0] = graphN;
   for (jj = 1; jj < nInputs; jj++)
   {
      graphN *= n1d;
      incrs[jj] = graphN;
   }
   graphI = new int[graphN+1];
   graphJ = new int[graphN*(nInputs-1)*2+1];
   nnz = 0;
   graphI[0] = nnz;
   for (ii = 0; ii < graphN; ii++)
   {
      itmp = ii;
      for (jj = 0; jj < nInputs-1; jj++)
      {
         jtmp = itmp % n1d;
         itmp = itmp / n1d;
         if (jtmp > 0     ) graphJ[nnz++] = ii - incrs[jj];
         if (jtmp < n1d-1) graphJ[nnz++] = ii + incrs[jj];
      }
      graphI[ii+1] = nnz;
   }
   cellsOccupied = new int[graphN];
   sample2Aggr = new int[nSamples];
   aggrMean = new double[nAggrs];
   aggrCnts = new int[nAggrs];

   options[0] = 0;
#ifdef HAVE_METIS
   METIS_PartGraphRecursive(&graphN, graphI, graphJ, NULL, NULL,
             &wgtflag,&numflag,&nAggrs,options,&edgeCut,cellsOccupied);
#else
   printf("Total Effect ERROR : METIS not installed.\n");
   nInputs = 0;
#endif

   tsi = new double[nInputs];
   for (inputID = 0; inputID < nInputs; inputID++)
   {
      for (ss = 0; ss < nSamples; ss++)
      {
         itmp = 0;
         for (ii = 0; ii < nInputs; ii++)
         {
            if (ii != inputID)
            {
               itmp = itmp * n1d;
               dtmp = X[ss*nInputs+ii];
               dtmp = (dtmp - lbounds[ii]) / ranges[ii];
               jtmp = (int) (dtmp * n1d);
               itmp += jtmp;
            }
         }
         sample2Aggr[ss] = cellsOccupied[itmp];
      }

      for (ii = 0; ii < nAggrs; ii++)
      {
         aggrMean[ii] = 0.0;
         aggrCnts[ii] = 0;
      }
      for (ss = 0; ss < nSamples; ss++)
      {
         index = sample2Aggr[ss];
         aggrMean[index] += Y[ss];
         aggrCnts[index]++;
      }
      totalCnt = 0;
      wFlag = 0;
      for (ii = 0; ii < nAggrs; ii++)
      {
         if (wFlag == 0 && aggrCnts[ii] == 0)
         {
            printf("TSIAnalyzer WARNING: some bins for input %d have no sample\n",inputID+1);
            printf("            points. This is unusual, and this may be due to\n");
            printf("            too many subdomains or that the sample points\n");
            printf("            are not distributed more or less evenly over\n");
            printf("            the parameter space.\n");
            wFlag = 1;
         }
      }
      for (ii = 0; ii < nAggrs; ii++)
         if (aggrCnts[ii] > 0) aggrMean[ii] /= (double) aggrCnts[ii];
      dmean = 0.0;
      for (ii = 0; ii < nAggrs; ii++)
          dmean += aggrMean[ii] * aggrCnts[ii];
      dmean /= (double) nSamples;
      dvar = 0.0;
      for (ii = 0; ii < nAggrs; ii++)
         dvar += pow(aggrMean[ii] - dmean, 2.0);
      dvar /= (double) (nAggrs - 1.0);

      if (dvar < variance)
         printf("Input %4d : Approximate total sensitivity index = %e\n",
                inputID+1, 1.0-dvar/variance);
      else
      {
         printf("Input %4d : Approximate total sensitivity index > variance?\n",
                inputID+1);
         printf("            Is your sample evenly distributed?\n");
         printf("            Do you have too many subdomains (too few in each)?\n");
      }
      tsi[inputID] = variance - dvar;
   }
   printResults(nInputs, variance, tsi, ioPtr);

   delete [] ranges;
   delete [] Y;
   delete [] aggrMean;
   delete [] aggrCnts;
   delete [] incrs;
   delete [] tsi;
   delete [] sample2Aggr;
   if (cellsOccupied != NULL) delete [] cellsOccupied;
   if (graphI != NULL) delete [] graphI;
   if (graphJ != NULL) delete [] graphJ;
   return 0;
}

// ************************************************************************
// equal operator
// ------------------------------------------------------------------------
TSIAnalyzer& TSIAnalyzer::operator=(const TSIAnalyzer &)
{
   printf("Total Effect operator= ERROR: operation not allowed.\n");
   exit(1);
   return (*this);
}

// ************************************************************************
// print result
// ------------------------------------------------------------------------
int TSIAnalyzer::printResults(int nInputs, double variance,
                              double *tsi, PsuadeData *ioPtr)
{
   int   ii;
   FILE  *fp;
   char  **iNames;
   pData qData;

   if (ioPtr != NULL) ioPtr->getParameter("input_names", qData);
   if (qData.strArray_ != NULL) iNames = qData.strArray_;
   else                         iNames = NULL;
   printEquals(0);
   if (variance == 0.0)
   {
      printf("Total variance = 0. Hence, no total effect plot.\n");
      return 0;
   }
   printf("Approximate Total Effect Statistics: \n");
   for (ii = 0; ii < nInputs; ii++)
      printf("Input %4d: Sobol' total sensitivity = %12.4e (normalized = %12.4e)\n",
             ii+1,tsi[ii],tsi[ii]/variance);
   if (psPlotTool_ == 1) fp = fopen("scilabtsi.sci", "w");
   else                  fp = fopen("matlabtsi.m", "w");
   if (fp != NULL)
   {
      if (psPlotTool_ == 1)
      {
         fprintf(fp, "// This file contains Sobol' total indices\n");
         fprintf(fp, "// set sortFlag = 1 and set nn to be the number\n");
         fprintf(fp, "// of inputs to display.\n");
      }
      else
      {
         fprintf(fp, "%% This file contains Sobol' total indices\n");
         fprintf(fp, "%% set sortFlag = 1 and set nn to be the number\n");
         fprintf(fp, "%% of inputs to display.\n");
      }
      fprintf(fp, "sortFlag = 0;\n");
      fprintf(fp, "nn = %d;\n", nInputs);
      fprintf(fp, "Mids = [\n");
      for (ii = 0; ii < nInputs; ii++) fprintf(fp,"%24.16e\n", tsi[ii]/variance);
      fprintf(fp, "];\n");
      if (iNames == NULL)
      {
         fprintf(fp, "  Str = {");
         for (ii = 0; ii < nInputs-1; ii++) fprintf(fp,"'X%d',",ii+1);
         fprintf(fp,"'X%d'};\n",nInputs);
      }
      else
      {
         fprintf(fp, "  Str = {");
         for (ii = 0; ii < nInputs-1; ii++)
         {
            if (iNames[ii] != NULL) fprintf(fp,"'%s',",iNames[ii]);
            else fprintf(fp,"'X%d',",ii+1);
         }
         if (iNames[nInputs-1] != NULL)
              fprintf(fp,"'%s'};\n",iNames[nInputs-1]);
         else fprintf(fp,"'X%d'};\n",nInputs);
      }
      fwritePlotCLF(fp);
      fprintf(fp, "if (sortFlag == 1)\n");
      if (psPlotTool_ == 1)
           fprintf(fp, "  [Mids, I2] = gsort(Mids);\n");
      else fprintf(fp, "  [Mids, I2] = sort(Mids,'descend');\n");
      fprintf(fp, "  Str  = Str(I2);\n");
      fprintf(fp, "  I2 = I2(1:nn);\n");
      fprintf(fp, "  Mids = Mids(1:nn);\n");
      fprintf(fp, "  Str  = Str(1:nn);\n");
      fprintf(fp, "end\n");
      fprintf(fp, "ymin = min(Mids);\n");
      fprintf(fp, "ymin = 0.0;\n");
      fprintf(fp, "ymax = max(Mids);\n");
      fprintf(fp, "h2 = 0.05 * (ymax - ymin);\n");
      fprintf(fp, "bar(Mids,0.8);\n");
      fwritePlotAxes(fp);
      if (psPlotTool_ == 1)
      {
         fprintf(fp, "a=gca();\n");
         fprintf(fp, "a.data_bounds=[0, ymin; nn+1, ymax];\n");
         fprintf(fp, "newtick = a.x_ticks;\n");
         fprintf(fp, "newtick(2) = [1:nn]';\n");
         fprintf(fp, "newtick(3) = Str';\n");
         fprintf(fp, "a.x_ticks = newtick;\n");
         fprintf(fp, "a.x_label.font_size = 3;\n");
         fprintf(fp, "a.x_label.font_style = 4;\n");
      }
      else
      {
         fprintf(fp, "axis([0 nn+1 ymin ymax])\n");
         fprintf(fp, "set(gca,'XTickLabel',[]);\n");
         fprintf(fp, "th=text(1:nn, repmat(ymin-0.07*(ymax-ymin),nn,1),Str,");
         fprintf(fp, "'HorizontalAlignment','left','rotation',90);\n");
         fprintf(fp, "set(th, 'fontsize', 12)\n");
         fprintf(fp, "set(th, 'fontweight', 'bold')\n");
      }
      fwritePlotTitle(fp,"Sobol Total Order Indices");
      fwritePlotYLabel(fp, "Sobol Indices");
      fclose(fp);
      if (psPlotTool_ == 1)
           printf("tsi plot file = scilabtsi.sci\n");
      else printf("tsi plot file = matlabtsi.m\n");
      return 0;
   }
   else
   {
      printf("TSIAnalyser ERROR: cannot create tsi plot file.\n");
      return 0;
   }
}

