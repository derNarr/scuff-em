#define HAVE_CONFIG_H
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
 * libscuff.h    -- header file for libscuff
 *
 * this file is long, but basically is divided into several digestible
 * sections:
 *
 *  1. class definitions for RWGSurface and supporting classes
 *  2. class definition for RWGGeometry
 *  3. non-class methods that operate on RWGPanels or RWGSurfaces
 *  4. other lower-level non-class methods 
 *
 * homer reid  -- 3/2007 -- 9/2011
 */

#ifndef LIBSCUFF_H
#define LIBSCUFF_H

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <complex>
#include <cmath>

#include <libhrutil.h>
#include <libhmat.h>
#include <libMatProp.h>
#include <libIncField.h>

#include "GTransformation.h"
#include "FieldGrid.h"

namespace scuff {

/*--------------------------------------------------------------*/
/*- some constants used to pass values to libscuff functions   -*/
/*--------------------------------------------------------------*/
#define SCUFF_GENERALFREQ  0
#define SCUFF_PUREIMAGFREQ 1

#define SCUFF_NOLOGGING      0
#define SCUFF_TERSELOGGING   1
#define SCUFF_VERBOSELOGGING 2

// various types of overlap matrix
#define SCUFF_OMATRIX_OVERLAP    0
#define SCUFF_OMATRIX_POWER      1
#define SCUFF_OMATRIX_XFORCE     2
#define SCUFF_OMATRIX_YFORCE     3
#define SCUFF_OMATRIX_ZFORCE     4
#define SCUFF_NUM_OMATRICES      5

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/* impedance of free space */
#ifndef ZVAC
#define ZVAC 376.73031346177
#endif

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*- 1. Class definitions for RWGSurface and supporting classes -*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/ 

/***************************************************************/
/* RWGPanel is a structure containing data on a single         */
/* triangular panel in the meshed geometry.                    */
/*                                                             */
/* note: after the following code snippet                      */
/*  double *R= O->Vertices + 3*O->Panels[np]->VI[i]            */
/* we have that R[0..2] are the cartesian coordinates of the   */
/* ith vertex (i=0,1,2) of the npth panel in surface O.        */
/*                                                             */
/***************************************************************/
typedef struct RWGPanel
 { 
   int VI[3];                   /* indices of vertices in Vertices array */
   double Centroid[3];          /* panel centroid */
   double ZHat[3];              /* normal vector */
   double Radius;               /* radius of enclosing sphere */
   double Area;                 /* panel area */
   int Index;                   /* index of this panel within RWGSurface (0..NumPanelsP-1)*/

} RWGPanel;

/***************************************************************/
/* RWGEdge is a structure containing data on a single          */
/* edge in a meshed surfaces.                                  */
/*                                                             */
/* This may be an *interior* edge, in which case all fields in */
/* the structure are valid; or it may be an *exterior* edge,   */
/* in which case the iQM, iMPanel, and MIndex fields all have  */
/* the value -1.                                               */
/*                                                             */
/* note: after the following code snippet                      */
/*  double *R= O->Vertices + 3*E->IQP                          */
/* we have that R[0..2] are the cartesian coordinates of the   */
/* QP vertex in the basis function corresponding to edge E.    */
/***************************************************************/
typedef struct RWGEdge 
 { 
   int iV1, iV2, iQP, iQM;	/* indices of panel vertices (iV1<iV2) */
   double Centroid[3];          /* edge centroid */
   double Length;               /* length of edge */
   double Radius;               /* radius of enclosing sphere */

   int iPPanel;                 /* index of PPanel within RWGSurface (0..NumPanelsP-1)*/
   int iMPanel;                 /* index of MPanel within RWGSurface (0..NumPanelsP-1)*/
   int PIndex;                  /* index of this edge within PPanel (0..2)*/
   int MIndex;                  /* index of this edge within MPanel (0..2)*/
   int Index;                   /* index of this edge within RWGSurface (0..NumEdges-1)*/

   RWGEdge *Next;               /* pointer to next edge in linked list */

} RWGEdge;

/***************************************************************/
/* fast kd-tree based point-in-object calculations             */
/***************************************************************/
typedef struct kdtri_s *kdtri;
void kdtri_destroy(kdtri t); // destructor

// some tree statistics for informational purposes:
unsigned kdtri_maxdepth(kdtri t);
size_t kdtri_maxleaf(kdtri t);
double kdtri_meandepth(kdtri t);
double kdtri_meanleaf(kdtri t);

/***************************************************************/
/* RWGSurface is a class describing a single contiguous surface*/
/* lying at the interface between two regions. The surface may */
/* be closed or open.                                          */
/***************************************************************/
class RWGSurface
 { 
   /*--------------------------------------------------------------*/ 
   /*- class methods ----------------------------------------------*/ 
   /*--------------------------------------------------------------*/ 
  public:  

   /* constructor entry point: construct from an 'OBJECT...ENDOBJECT' */
   /* or SURFACE...ENDSURFACE section in a .scuffgeo file             */
   RWGSurface(FILE *f, const char *Label, int *LineNum, char *Keyword);

   /* alternative constructor entry point: construct from fixed lists */
   /* of vertices and panels. This defines an RWGSurface which exists */
   /* independently of any RWGGeometry structure, which means it      */
   /* doesn't know what regions it bounds or what materials are on    */
   /* either side of it, but can still be used for low-level          */
   /* computations at the level of individual basis functions and     */
   /* panels.                                                         */
   /* In this case, Vertices[3*nv+0, 3*nv+1, 3*nv+2] are the cartesian*/
   /* coordinates of the nvth vertex, and the vertex indices of the   */
   /* npth panel should be PanelVertices[3*np, 3*np+1, 3*np+2].       */
   RWGSurface(double *Vertices, int NumVertices, int *PanelVertices, int NumPanels);

   /* destructor */
   ~RWGSurface();

   /* get overlap integrals between two basis functions */
   double GetOverlap(int neAlpha, int neBeta, double *pOTimes = NULL);
   void GetOverlaps(int neAlpha, int neBeta, double *Overlaps);

   /* get one or more overlap matrices for the surface as a whole*/
   /* note: NeedMatrix and SArray are arrays of length SCUFF_NUM_OMATRICES */
   void GetOverlapMatrices(int *NeedMatrix, SMatrix **SArray, 
                           cdouble Omega=1.0, MatProp *ExteriorMP=NULL);

   /* apply a general transformation (rotation+displacement) to the surface */
   void Transform(const GTransformation *GT);
   void Transform(char *format, ...);
   void UnTransform();

   /* fast inclusion tests */
   bool Contains(const double X[3]);
   bool Contains(const RWGSurface *S);

   /* visualization */
   // void Visualize(double *KVec, double Kappa, char *format, ...);
   void WriteGPMesh(const char *format, ...);
   void WritePPMesh(const char *FileName, const char *Tag, int PlotNormals=0);
   void WritePPMeshLabels(const char *FileName, const char *Tag, int WhichLabels);
   void WritePPMeshLabels(const char *FileName, const char *Tag);

//  private:

   /*--------------------------------------------------------------*/
   /*- private data fields  ---------------------------------------*/
   /*--------------------------------------------------------------*/
   char *RegionLabels[2];          /* names of the regions on either side of the surface */
   int RegionIndices[2];           /* indices of the two regions within the RWGGeometry list of Regions */
   int IsPEC;                      /* =1 if this is a simple PEC object */
   int IsObject;                   /* =1 if we came from an OBJECT...ENDOBJECT section */
                                   /* =0 if we came from a SURFACE...ENDSURFACE section */

   double *Vertices;               /* Vertices[3*n,3*n+1,3*n+2]=nth vertex coords */
   RWGPanel **Panels;              /* array of pointers to panels         */
   RWGEdge **Edges;                /* array of pointers to edges          */
   RWGEdge **ExteriorEdges;        /* array of pointers to exterior edges */
   int IsClosed;                   /* = 1 for a closed surface, 0 for an open surface */
   int HaveLineCharges;            /* = 1 if any surface in the geometry has line charges */

   int NumVertices;                /* number of vertices in mesh  */
   int NumInteriorVertices;        /* number of interior vertices */
   int NumRefPts;                  /* number of vertices used as reference points */

   int NumTotalEdges;              /* total number of edges */
   int NumEdges;                   /* number of interior edges */
   int NumExteriorEdges;           /* number of exterior edges */

   int NumBFs;                     /* number of basis functions */
   int NumPanels;                  /* number of panels */
   int NumBCs;                     /* number of boundary countours */

   int Index;                      /* index of this surface in geometry  */

   int *WhichBC;                   /* WhichBC[nv] = index of boundary contour */
                                   /* on which vertex #nv lies (=0 if vertex  */
                                   /* #nv is an internal vertex)              */
   int *NumBCEdges;                /* NumBCEdges[2] is the number of edges in */
                                   /* boundary contour #2                     */
   RWGEdge ***BCEdges;             /* BCEdges[2][3] is a pointer to the 3rd   */
                                   /* edge in boundary contour #2             */

   int NumRedundantVertices;

   char *MeshFileName;             /* saved name of mesh file */
   int PhysicalRegion;             /* index of surface within mesh file; = -1 if not applicable */
   char *Label;                    /* unique label identifying surface */

   kdtri kdPanels; /* kd-tree of panels */
   void InitkdPanels(bool reinit = false, int LogLevel = SCUFF_NOLOGGING);

   /* GT encodes any transformation that has been carried out since */
   /* the surface was read from its mesh file (not including a      */
   /* possible one-time GTransformation that may have been specified*/
   /* in the .scuffgeo file when the surface was first created.)    */
   GTransformation *GT;

   /* SurfaceSigma, if non-NULL, points to a cevaluator for a     */
   /* user-specified function of frequency and position (w,x,y,z) */
   /* describing surface conductivity                             */
   void *SurfaceSigma;

   // the following fields are used to pass some data items up to the 
   // higher-level routine that calls the RWGSurface constructor
   char *ErrMsg;                   /* used to indicate to a calling routine that an error has occurred */
   int MaterialRegionsLineNum;     /* line of .scuffgeo file on which MATERIAL or REGIONS keyword appeared */
   char *MaterialName;             /* name of material in OBJECT...ENDOBJECT section */

   /*--------------------------------------------------------------*/ 
   /*- private class methods --------------------------------------*/ 
   /*--------------------------------------------------------------*/ 
   /* the actual body of the class constructor */
   void InitRWGSurface(const GTransformation *OTGT=0);

   /* constructor subroutines */
   void InitEdgeList();
   void ReadGMSHFile(FILE *MeshFile, char *FileName, const GTransformation *GT, int PhysicalRegion);
   void ReadComsolFile(FILE *MeshFile, char *FileName, const GTransformation *GT);

   /* calculate reduced potentials due to a single basis function */
   /* (this is a helper function used to implement the            */
   /*  GetInnerProducts() class method)                           */
   void GetReducedPotentials(int ne, const double *X, cdouble K,
                             cdouble *a, cdouble *Curla, cdouble *Gradp);
 
 };

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*- 3. Class definition for RWGGeometry                        -*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

/*************************** ************************************/
/* an RWGGeometry is a collection of regions with interfaces   */
/* described by RWGSurfaces.                                   */
/***************************************************************/
class RWGGeometry 
 { 
   /*--------------------------------------------------------------*/ 
   /*- public class methods ---------------------------------------*/ 
   /*--------------------------------------------------------------*/ 
  public:  

   /* constructor / destructor */
   RWGGeometry(const char *GeoFileName, int pLogLevel = SCUFF_NOLOGGING);
   ~RWGGeometry();

   /* geometrical transformations */
   void Transform(GTComplex *GTC);
   void UnTransform();
   char *CheckGTCList(GTComplex **GTCList, int NumGTCs);

   /* visualization */
   void WritePPMesh(const char *FileName, const char *Tag, int PlotNormals=0);
   void WriteGPMesh(const char *format, ...);
   void WriteGPMeshPlus(const char *format, ...);
   void PlotSurfaceCurrents(HVector *KN, cdouble Omega, const char *format, ...);

   /* routines for allocating, and then filling in, the BEM matrix */
   HMatrix *AllocateBEMMatrix(bool PureImagFreq = false, bool Packed = false);
   HMatrix *AssembleBEMMatrix(cdouble Omega, HMatrix *M = NULL);

   /* routines for allocating, and then filling in, the RHS vector */
   HVector *AllocateRHSVector(bool PureImagFreq = false );
   HVector *AssembleRHSVector(cdouble Omega, IncField *IF, HVector *RHS = NULL);

   int UpdateIncFields(IncField *IF, cdouble Omega);

   // get the index of the region containing point X
   int GetRegionIndex(const double X[3]);
   int PointInRegion(int RegionIndex, const double X[3]);

   /* simplest routine for computing fields */
   void GetFields(IncField *IF, HVector *KN, cdouble Omega, double *X, cdouble *EH);

   /* more sophisticated routine for computing fields */
   HMatrix *GetFields(IncField *IF, HVector *KN,
                      cdouble Omega, HMatrix *XMatrix,
                      HMatrix *FMatrix=NULL, char *FuncString=NULL);

   /****************************************************/
   /* Routine for evaluating arbitrary functions of the fields on a 2d
      surface grid; see FieldGrid.h/cc.  Returns a NULL-terminated
      (malloc'ed) array of HMatrix pointers.  If a non-NULL incident
      field function is supplied, then the total of scattered plus
      incident fields is used.  If KN == NULL, then the scattered
      fields are set to zero. */
   HMatrix **GetFieldsGrids(SurfaceGrid &grid, int nfuncs, FieldFunc **funcs,
			    cdouble Omega, HVector *KN = NULL,
			    IncField *inc=NULL);

   // exprs is a string of COMMA-SEPARATED expressions
   HMatrix **GetFieldsGrids(SurfaceGrid &grid, const char *exprs,
                            cdouble Omega, HVector *KN=NULL, 
                            IncField *inc=NULL);

   // variants that only compute one function and return one matrix
   HMatrix *GetFieldsGrid(SurfaceGrid &grid, FieldFunc &func,
			  cdouble Omega, HVector *KN=NULL, IncField *inc=NULL);
   HMatrix *GetFieldsGrid(SurfaceGrid &grid, const char *expr,
			  cdouble Omega, HVector *KN=NULL, IncField *inc=NULL);

   /* routine for computing power, force, and torque on an object */
   void GetPFT(HVector *KN, HVector *RHS, cdouble Omega, int SurfaceIndex, double PFT[8]);
   void GetPFT(HVector *KN, HVector *RHS, cdouble Omega, char *SurfaceLabel, double PFT[8]);

   /* routine for computing scattered power */
   cdouble GetScatteredPower(HVector *KN, cdouble Omega, int SurfaceIndex);
   cdouble GetScatteredPower(HVector *KN, cdouble Omega, char *SurfaceLabel);

   /* routine for calculating electric and magnetic dipole moments */
   HVector *GetDipoleMoments(cdouble Omega, HVector *KN, HVector *PM=0);

   /* routine for computing the expansion coefficients in the RWG basis */
   /* of an arbitrary user-supplied surface-tangential vector field;    */
   void ExpandCurrentDistribution(IncField *IF, HVector *KN);

   /* evaluate the surface currents at a given point X on an object */
   /* surface, given a vector of RWG expansion coefficients         */
   void EvalCurrentDistribution(const double X[3], HVector *KNVec, cdouble KN[6]);

   /* routine for setting logging verbosity */
   void SetLogLevel(int LogLevel);

   /* routines for changing material properties */
   void SetEps(cdouble Eps);
   void SetEps(const char *Label, cdouble Eps);
   void SetEpsMu(cdouble Eps, cdouble Mu);
   void SetEpsMu(const char *Label, cdouble Eps, cdouble Mu);

   /* some simple utility functions */
   int GetDimension();
   int GetRegionByLabel(const char *Label);
   RWGSurface *GetSurfaceByLabel(const char *Label, int *pns=NULL);

   /*--------------------------------------------------------------------*/ 
   /*- class methods intended for internal use only, i.e. which          */ 
   /*- would be private if we cared about the public/private distinction */
   /*--------------------------------------------------------------------*/ 
   // constructor helper functions
   void ProcessMEDIUMSection(FILE *f, char *FileName, int *LineNum);
   void AddRegion(char *RegionLabel, char *MaterialName, int LineNum);

   // helper functions for AssembleBEMMatrix
   int CountCommonRegions(int nsa, int nsb, double Signs[2], cdouble Eps[2], cdouble Mu[2]);
   void AddLineChargeContributionsToBEMMatrix(cdouble Omega, HMatrix *M);
   void UpdateCachedEpsMuValues(cdouble Omega);

   /*--------------------------------------------------------------*/ 
   /*- private data fields  ---------------------------------------*/ 
   /*--------------------------------------------------------------*/ 
//private:

   int NumRegions;
   char **RegionLabels;
   MatProp **RegionMPs;

   // cached values of epsilon and mu for each region
   // 'EpsTF' = 'epsilon, this frequency'
   cdouble *EpsTF, *MuTF;
   cdouble StoredOmega;

   int NumSurfaces;
   RWGSurface **Surfaces;
   int AllSurfacesClosed;
   int HaveLineCharges;

   int TotalBFs;
   int TotalPanels;
   double AveragePanelArea;

   int Verbose;

   char *GeoFileName;

   /* BFIndexOffset[n] is the index within the overall BEM          */
   /* system vector of the first basis function on surface #n. thus */
   /*  BFIndexOffset[0]=0                                           */
   /*  BFIndexOffset[1]=# BFs for surface 0                         */
   /*  BFIndexOffset[2]=# BFs for surface 0 + # BFs for surface     */
   /* etc.                                                          */
   int *BFIndexOffset;
   int *PanelIndexOffset;

   /* Mate[i]=j if j<i and surface j is identical to surface i. */
   /* Mate[i]=-1 if there is no surface identical to surface i, */
   /* OR if there is an surface identical to surface i but its  */
   /* index is greater than i.                                  */
   /* for example, if surfaces 2 and 3 are identical then       */
   /*  Mate[2]=-1, Mate[3]=2.                                   */
   /* note: two surfaces are identical if  	                */
   /*  (1) they have the same mesh file (and the same physical  */
   /*      region in that mesh file)                            */
   /*  (2) they have the same Regions on the er side            */
   int *Mate;

   /* SurfaceMoved[i] = 1 if surface #i was moved on the most   */
   /* recent call to Transform(). otherwise SurfaceMoved[i]=0.  */
   int *SurfaceMoved;
  
   int LogLevel; 
   
   static bool AssignBasisFunctionsToExteriorEdges;
   static bool IncludeLineChargeContributions;

 };

/***************************************************************/
/* non-class methods that operate on RWGPanels                 */
/***************************************************************/
RWGPanel *NewRWGPanel(double *Vertices, int iV1, int iV2, int iV3);
void InitRWGPanel(RWGPanel *P, double *Vertices);

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*- 4. some other lower-level non-class methods                -*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
  
/* 3D vector manipulations */
void VecZero(double v[3]);
double *VecCopy(const double v1[3], double v2[3]);
double *VecScale(const double v1[3], double alpha, double v2[3]);
double *VecScale(double v[3], double alpha);
double *VecScaleAdd(const double v1[3], double alpha, const double v2[3], double v3[3]);
double *VecAdd(const double v1[3], const double v2[3], double v3[3]);
double *VecSub(const double v1[3], const double v2[3], double v3[3]);
double *VecPlusEquals(double v1[3], double alpha, const double v2[3]);
double *VecCross(const double v1[3], const double v2[3], double v3[3]);
double *VecLinComb(double alpha, const double v1[3], double beta, const double v2[3], 
                   double v3[3]);
double VecDot(const double v1[3], const double v2[3]);
double VecDistance(const double v1[3], const double v2[3]);
double VecDistance2(const double v1[3], const double v2[3]);
double VecNorm(const double v[3]);
double VecNorm2(const double v[3]);
double VecNormalize(double v[3]);
bool EqualFloat(const double a, const double b);
bool VecEqualFloat(const double *a, const double *b);

void SixVecPlus(const cdouble V1[6], const cdouble Alpha,
                const cdouble V2[6], cdouble V3[6]);
void SixVecPlusEquals(cdouble V1[6], const cdouble Alpha, const cdouble V2[6]);
void SixVecPlusEquals(cdouble V1[6], const cdouble V2[6]);

/* routines for creating the 'Gamma Matrix' used for torque calculations */
void CreateGammaMatrix(double *TorqueAxis, double *GammaMatrix);
void CreateGammaMatrix(double TorqueAxisX, double TorqueAxisY, 
                       double TorqueAxisZ, double *GammaMatrix);
void CreateGammaMatrix(double Theta, double Phi, double *GammaMatrix);

cdouble ExpRel(int n, cdouble Z);

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
void PreloadCache(const char *FileName);
void StoreCache(const char *FileName);

} // namespace scuff

#endif // #ifndef LIBSCUFF_H
