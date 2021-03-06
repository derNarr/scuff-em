/* Copyright (C) 2005-2011 M. T. Homer Reid
 *
 * This file is part of SCUFF-EM.
 *
 * SCUFF-EM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SCUFF-EM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * GetDyadicGF.cc  -- compute the scattering parts of the electric and
 *                    magnetic dyadic Green's functions
 *
 * homer reid      -- 5/2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

#include <libhrutil.h>

#include "libscuff.h"
#include "libscuffInternals.h"

#define II cdouble(0.0,1.0)

namespace scuff {

/***************************************************************/
/* This routine compute the scattering parts of the electric   */
/* and magnetic dyadic green's functions (DGFs) at a point X.  */
/*                                                             */
/* The calculation proceeds by (a) placing a point dipole      */
/* source at X, (b) solving a scattering problem with the      */
/* incident field emanating from this point source, and then   */
/* (c) evaluating the scattered fields back at X.              */
/*                                                             */
/* If the original point dipole source was an electric dipole  */
/* pointing in the i direction, then the scattered E-field at  */
/* X gives us the ith column of the electric DGF.              */
/*                                                             */
/* If the original point dipole source was a *magnetic* dipole */
/* pointing in the i direction, then the scattered H-field at  */
/* X gives us the ith column of the magnetic DGF.              */
/*                                                             */
/* Inputs:                                                     */
/*                                                             */
/*  X:     cartesian coordinates of evaluation point           */
/*                                                             */
/*  Omega: angular frequency                                   */
/*                                                             */
/*  M:     the LU-factorized BEM matrix. Note that the caller  */
/*         is responsible for calling AssembleBEMMatrix() and  */
/*         LUFactorize() before calling this routine.          */
/*                                                             */
/*  KN:    an HVector used internally within this routine that */
/*         that must have been obtained from a previous call   */
/*         to AllocateRHSVector().                             */
/*                                                             */
/* Outputs:                                                    */
/*                                                             */
/*  On return, GE[i][j] and GM[i][j] are respectively the      */
/*  i,j components of the electric and magnetic DGF tensors.   */
/*                                                             */
/***************************************************************/
void RWGGeometry::GetDyadicGFs(double X[3], cdouble Omega, HMatrix *M, HVector *KN,
                               cdouble GE[3][3], cdouble GM[3][3])
{
  if (M==0 || M->NR != TotalBFs || M->NC!=M->NR )
   ErrExit("%s:%i: invalid M matrix passed to GetDyadicGFs()",__FILE__,__LINE__);

  if (KN==0 || KN->N != TotalBFs )
   ErrExit("%s:%i: invalid K vector passed to GetDyadicGFs()",__FILE__,__LINE__);

  /***************************************************************/
  /***************************************************************/
  /***************************************************************/
  cdouble P[3]={1.0, 0.0, 0.0};
  PointSource PS(X, P);
  cdouble EH[6];

  cdouble Eps, Mu;
  int nr=GetRegionIndex(X);
  RegionMPs[nr]->GetEpsMu(Omega, &Eps, &Mu);
  //cdouble IKZ = II*Omega*Mu*ZVAC;
  cdouble k2 = Eps*Mu*Omega*Omega;
  cdouble Z2 = ZVAC*ZVAC*Mu/Eps;

  for(int i=0; i<3; i++)
   { 
     // set point source to point in the ith direction
     memset(P, 0, 3*sizeof(cdouble));
     P[i]=1.0;
     PS.SetP(P);

     // solve the scattering problem for an electric point source 
     PS.SetType(LIF_ELECTRIC_DIPOLE);
     AssembleRHSVector(Omega, &PS, KN);
     M->LUSolve(KN);
     GetFields(0, KN, Omega, X, EH);
     GE[0][i]=EH[0] / k2;
     GE[1][i]=EH[1] / k2;
     GE[2][i]=EH[2] / k2;

     // solve the scattering problem for a magnetic point source 
     PS.SetType(LIF_MAGNETIC_DIPOLE);
     AssembleRHSVector(Omega, &PS, KN);
     M->LUSolve(KN);
     GetFields(0, KN, Omega, X, EH);
     GM[0][i]=EH[3] * Z2/k2;
     GM[1][i]=EH[4] * Z2/k2;
     GM[2][i]=EH[5] * Z2/k2;
   };
}

/***************************************************************/
/***************************************************************/
/***************************************************************/
void RWGGeometry::GetDyadicGFs(double XEval[3], double XSource[3], 
                               cdouble Omega, HMatrix *M, HVector *KN,
                               cdouble GEScat[3][3], cdouble GMScat[3][3],
                               cdouble GETot[3][3], cdouble GMTot[3][3])
{
  if (M==0 || M->NR != TotalBFs || M->NC!=M->NR )
   ErrExit("%s:%i: invalid M matrix passed to GetDyadicGFs()",__FILE__,__LINE__);

  if (KN==0 || KN->N != TotalBFs )
   ErrExit("%s:%i: invalid K vector passed to GetDyadicGFs()",__FILE__,__LINE__);

  /***************************************************************/
  /***************************************************************/
  /***************************************************************/
  cdouble P[3]={1.0, 0.0, 0.0};
  PointSource PS(XSource, P);
  cdouble EH[6];

  cdouble Eps, Mu;
  int nr=GetRegionIndex(XSource);
  RegionMPs[nr]->GetEpsMu(Omega, &Eps, &Mu);
  cdouble Z2 = ZVAC*ZVAC*Mu/Eps;
  cdouble k2 = Eps*Mu*Omega*Omega;

  for(int i=0; i<3; i++)
   { 
     // set point source to point in the ith direction
     memset(P, 0, 3*sizeof(cdouble));
     P[i]=1.0;
     PS.SetP(P);

     // solve the scattering problem for an electric point source 
     PS.SetType(LIF_ELECTRIC_DIPOLE);
     AssembleRHSVector(Omega, &PS, KN);
     M->LUSolve(KN);
     GetFields(0, KN, Omega, XEval, EH);
     GEScat[0][i]=EH[0] / k2;
     GEScat[1][i]=EH[1] / k2;
     GEScat[2][i]=EH[2] / k2;

     PS.GetFields(XEval, EH);
     GETot[0][i]=GEScat[0][i] + EH[0] / k2;
     GETot[1][i]=GEScat[1][i] + EH[1] / k2;
     GETot[2][i]=GEScat[2][i] + EH[2] / k2;

     // solve the scattering problem for a magnetic point source 
     PS.SetType(LIF_MAGNETIC_DIPOLE);
     AssembleRHSVector(Omega, &PS, KN);
     M->LUSolve(KN);
     GetFields(0, KN, Omega, XEval, EH);
     GMScat[0][i]=EH[3] * Z2/k2;
     GMScat[1][i]=EH[4] * Z2/k2;
     GMScat[2][i]=EH[5] * Z2/k2;

     PS.GetFields(XEval, EH);
     GMTot[0][i]=GMScat[0][i] + EH[3] * Z2/k2;
     GMTot[1][i]=GMScat[1][i] + EH[4] * Z2/k2;
     GMTot[2][i]=GMScat[2][i] + EH[5] * Z2/k2;

   };
}

} // namespace scuff
