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
 * RWGGeometry.cc -- implementation of some methods in the RWGGeometry class
 *
 * homer reid      -- 3/2007 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

#include <libhrutil.h>

#include "libscuff.h"

namespace scuff {

#define MAXSTR 1000
#define MAXTOK 50

bool RWGGeometry::AssignBasisFunctionsToExteriorEdges=true;
bool RWGGeometry::IncludeLineChargeContributions=true;

/***********************************************************************/
/* subroutine to parse the MEDIUM...ENDMEDIUM section in a .scuffgeo   */
/* file. (currently the only keyword supported for this section is     */
/* MATERIAL xx).                                                       */
/* note: this is legacy syntax for changing the material properties of */
/* the exterior region; the modern way to do it would be to use a      */
/* line like REGION Exterior MATERIAL SiC.                             */
/***********************************************************************/
void RWGGeometry::ProcessMEDIUMSection(FILE *f, char *FileName, int *LineNum)
{
  char Line[MAXSTR];
  int NumTokens;
  char *Tokens[MAXTOK];
  while( fgets(Line,MAXSTR,f) )
   { 
     (*LineNum)++;
     NumTokens=Tokenize(Line, Tokens, MAXTOK);
     if ( NumTokens==0 || Tokens[0][0]=='#' )
      continue; 

     if ( !strcasecmp(Tokens[0],"MATERIAL") )
      {
        if (NumTokens!=2)
         ErrExit("%s:%i: syntax error",FileName,*LineNum);
        RegionMPs[0] = new MatProp(Tokens[1]); 
        if (RegionMPs[0]->ErrMsg)
         ErrExit("%s:%i: %s",FileName,*LineNum,RegionMPs[0]->ErrMsg);
        Log("Setting material properties of exterior region to %s.",Tokens[1]);
      }
     else if ( !strcasecmp(Tokens[0],"ENDMEDIUM") )
      { 
        return;
      }
     else
      {
        ErrExit("%s:%i: unknown keyword %s",FileName,*LineNum,Tokens[0]);
      };
     
   };

  ErrExit("%s: unexpected end of file",FileName);

}

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
void RWGGeometry::AddRegion(char *RegionLabel, char *MaterialName, int LineNum)
{
  int RegionID; 

  /*--------------------------------------------------------------*/
  /*- first check to see if the region label is 'Exterior', in   -*/
  /*- which case we aren't adding a new region but just redefining*/
  /*- the properties of the exterior region.                      */
  /*--------------------------------------------------------------*/
  if ( !strcasecmp(RegionLabel,"EXTERIOR") )
   { 
     RegionID = 0; 
   }
  else if ( !strcasecmp(RegionLabel,"PEC") )
   { 
     ErrExit("%s:%i: REGIONS cannot be PEC (use an OBJECT instead)",GeoFileName,LineNum);
   }
  else
   { 
     RegionLabels = (char **)reallocEC( RegionLabels, (NumRegions+1)*sizeof(char *));
     RegionLabels[NumRegions] = strdup(RegionLabel);
     RegionMPs = (MatProp **)reallocEC( RegionMPs, (NumRegions)*sizeof(MatProp *));
     NumRegions++;

     RegionID = NumRegions-1;
   };

  RegionMPs[RegionID] = new MatProp(MaterialName);
  if ( RegionMPs[RegionID]->ErrMsg )
   ErrExit("%s:%i: %s\n",GeoFileName,LineNum,RegionMPs[RegionID]->ErrMsg);

}


/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
RWGGeometry::RWGGeometry(const char *pGeoFileName, int pLogLevel)
{ 
  /***************************************************************/
  /* NOTE: i am not sure where to put this. put it here for now. */
  /***************************************************************/
  MatProp::SetLengthUnit(1.0e-6);
   
  /***************************************************************/
  /* initialize simple fields ************************************/
  /***************************************************************/
  LogLevel=pLogLevel;
  NumSurfaces=TotalPanels=TotalBFs=0;
  GeoFileName=strdup(pGeoFileName);
  Surfaces=0;
  AllSurfacesClosed=1;
  HaveLineCharges=0;

  // we always start with a single Region, for the exterior,
  // taken to be vacuum by default
  NumRegions=1;
  RegionLabels=(char **)mallocEC(1*sizeof(char *));
  RegionLabels[0]=strdup("EXTERIOR");
  RegionMPs=(MatProp **)mallocEC(1*sizeof(MatProp *));
  RegionMPs[0] = new MatProp("VACUUM");

  /***************************************************************/
  /* try to open input file **************************************/
  /***************************************************************/
  FILE *f=fopen(GeoFileName,"r");
  if (!f)
   ErrExit("could not open %s",GeoFileName);

  /***************************************************************/
  /* read and process lines from input file one at a time        */
  /***************************************************************/
  RWGSurface *S, *SP;
  char Line[MAXSTR];
  int LineNum=0; 
  int nTokens;
  char *Tokens[MAXTOK];
  while( fgets(Line,MAXSTR,f) )
   { 
     LineNum++;

     /*--------------------------------------------------------------*/
     /*- break up line into tokens; skip blank lines and comments ---*/
     /*--------------------------------------------------------------*/
     nTokens=Tokenize(Line, Tokens, MAXTOK);
     if ( nTokens==0 || Tokens[0][0]=='#' )
      continue; 
    
     /*--------------------------------------------------------------*/
     /*- switch off based on first token on the line ----------------*/
     /*--------------------------------------------------------------*/
     if ( !strcasecmp(Tokens[0],"MEDIUM") )
      { 
        ProcessMEDIUMSection(f,GeoFileName,&LineNum);
      }
     else if ( !strcasecmp(Tokens[0],"MATERIAL") )
      {
        /*--------------------------------------------------------------*/
        /* hand off to MatProp class constructor to parse this section  */
        /*--------------------------------------------------------------*/
        if ( nTokens==1 )
         ErrExit("%s:%i: no name given for MATERIAL ",GeoFileName,LineNum);
        else if ( nTokens>2 )
         ErrExit("%s:%i: syntax error",GeoFileName,LineNum);
         
        char *ErrMsg=AddMaterialToMatPropDataBase(f, GeoFileName, Tokens[1], &LineNum);
        if (ErrMsg)
         ErrExit("%s:%i: %s",GeoFileName,LineNum,ErrMsg); 

      }
     else if ( !strcasecmp(Tokens[0],"REGION") )
      {
        /*--------------------------------------------------------------*/
        /*- add a new region to our geometry ---------------------------*/
        /*--------------------------------------------------------------*/
        if ( nTokens!=4 || strcasecmp(Tokens[2],"MATERIAL") )
         ErrExit("%s:%i: syntax error",GeoFileName,LineNum);
        AddRegion(Tokens[1], Tokens[3], LineNum);
      }
     else if ( !strcasecmp(Tokens[0],"OBJECT") || !strcasecmp(Tokens[0],"SURFACE") )
      { 
        if ( nTokens==2 )
         S=new RWGSurface(f,Tokens[1],&LineNum,Tokens[0]);
        else if (nTokens!=1)
         ErrExit("%s:%i: syntax error",GeoFileName,LineNum);
        else
         { 
           char Label[MAXSTR];
           snprintf(Label,MAXSTR,"Surface_%i",NumSurfaces+1);
           S=new RWGSurface(f,Label,&LineNum, Tokens[0]);
         };

        if (S->ErrMsg)
         ErrExit("%s:%i: %s",GeoFileName,LineNum,S->ErrMsg); 

        if ( S->IsObject )
         { 
            /* for an OBJECT, if it is non-PEC, we need to add a new Region for  */
            /* the region interior to the object. In this case, for now, we set  */
            /* S->RegionLabels[0] = EXTERIOR and S->RegionLabels[1] = the label  */
            /* specified for the object in question. We may subsequently need to */
            /* change S->RegionLabels[0] (and S->RegionIndices[0]) to another    */
            /* region if we detect that the object is embedded in another object.*/
            S->RegionIndices[0] = 0;
            if ( S->IsPEC )
             S->RegionIndices[1] = -1;
            else
             { AddRegion(S->RegionLabels[1], S->MaterialName, S->MaterialRegionsLineNum );
               S->RegionIndices[1] = NumRegions - 1 ;
             };
         }
        else
         { 
           AllSurfacesClosed=0;

           /* On the other hand, for a SURFACE, we need to check that the       */
           /* REGIONS specified in the SURFACE...ENDSURFACE description         */
           /* are regions that have been previously declared.                   */
           S->RegionIndices[0] = GetRegionByLabel(S->RegionLabels[0]);
           if (S->RegionIndices[0]==-1)
            ErrExit("%s:%i: unknown region %s",GeoFileName,S->MaterialRegionsLineNum,S->RegionLabels[0]);

           if( S->IsPEC )
            S->RegionIndices[1] = -1;
           else
            { S->RegionIndices[1] = GetRegionByLabel(S->RegionLabels[1]);
              if (S->RegionIndices[1]==-1)
               ErrExit("%s:%i: unknown region %s",GeoFileName,S->MaterialRegionsLineNum,S->RegionLabels[1]);
            };
         };

        NumSurfaces++;
        Surfaces=(RWGSurface **)realloc(Surfaces, NumSurfaces*sizeof(RWGSurface *) );
        Surfaces[NumSurfaces-1]=S;

        TotalBFs+=S->NumBFs;
        TotalPanels+=S->NumPanels;
        S->Index=NumSurfaces-1;

      }
     else 
      { 
        /*--------------------------------------------------------------*/
        /* unknown keyword                                              */
        /*--------------------------------------------------------------*/
        ErrExit("%s:%i: syntax error",GeoFileName,LineNum);
      };

   }; // while( fgets(Line,MAXSTR,f) )

  /*--------------------------------------------------------------*/
  /*- set a flag if there are line charges present. --------------*/
  /*- line charges are present iff:                              -*/
  /*-  (1) one or more surface contains exterior edges, and      -*/
  /*-  (2) we are assigning half-RWG basis functions to those    -*/
  /*-      exterior edges (as opposed to simply ignoring them).  -*/
  /*--------------------------------------------------------------*/
  if( IncludeLineChargeContributions )
   { for(int ns=0; ns<NumSurfaces; ns++)
      if ( Surfaces[ns]->NumExteriorEdges > 0 )
       HaveLineCharges=1;
   };


  /*--------------------------------------------------------------*/
  /* Autodetect nesting relationships & topologically sort        */
  /* (so that if A contains B, then B comes after A).             */
  /* Note that the Contains() function and the autodetection of   */
  /* containership relations is only applicable to CLOSED         */
  /* surfaces; for OPEN surfaces there is no autodetection, and   */
  /* we rely instead on the user to specify explicitly which      */
  /* surfaces bound which regions.                                */
  /*--------------------------------------------------------------*/
  for (int ns = 0; ns < NumSurfaces; ++ns)
   Surfaces[ns]->InitkdPanels(false, LogLevel);
  for (int ns = 1; ns < NumSurfaces; ++ns) 
   { 
     S=Surfaces[ns];
     int NewSlotForS=-1;
     for (int nsp = ns-1; nsp >= 0; --nsp) // innermost to outermost
      { 
        SP = Surfaces[nsp];
        if ( SP->Contains(S) ) 
         {
           if (SP->IsPEC) 
            ErrExit("%s: PEC object %s cannot contain object %s",GeoFileName,SP->Label,S->Label);

	   // if SP contains S, then we set the exterior region for S to be
	   // the interior region for SP.
	   S->RegionIndices[0] = SP->RegionIndices[1];
	   free(S->RegionLabels[0]);
	   S->RegionLabels[0]=strdup(SP->RegionLabels[1]);
	   break;
         } 
        else if ( S->Contains(SP) )
         NewSlotForS = nsp;
      };

     if ( NewSlotForS != -1 )
      {  
        // for each surface between the current occupant of slot #NewSlotForS  
        // and slot #ns, move the surface one slot forward in the array and
        // adjust its region ID as necessary
        for(int nsp = ns; nsp > NewSlotForS; --nsp) 
         { 
           Surfaces[nsp] = Surfaces[nsp-1];

           SP = Surfaces[nsp];
           if (    SP->RegionIndices[0] == S->RegionIndices[0] 
                && S->Contains(SP) 
              ) 
            { 
              if (S->IsPEC) 
               ErrExit("%s: PEC object %s cannot contain object %s",GeoFileName,S->Label,SP->Label);
              SP->RegionIndices[0] = S->RegionIndices[1];
	      free(SP->RegionLabels[0]);
	      SP->RegionLabels[0]=strdup(S->RegionLabels[1]);
            };
         };
        Surfaces[NewSlotForS] = S;
      };

    }; // for(ns = 1 ...)
 
  /*******************************************************************/
  /* compute average panel area for statistical bookkeeping purposes */
  /*******************************************************************/
  AveragePanelArea=0.0; 
  for(int ns=0; ns<NumSurfaces; ns++)
   for(int np=0; np<Surfaces[ns]->NumPanels; np++)
    AveragePanelArea+=Surfaces[ns]->Panels[np]->Area;
  AveragePanelArea/=((double) TotalPanels);

  /***************************************************************/
  /* initialize arrays of basis-function and panel index offsets */
  /***************************************************************/
  BFIndexOffset=(int *)mallocEC(NumSurfaces*sizeof(int) );
  PanelIndexOffset=(int *)mallocEC(NumSurfaces*sizeof(int) );
  BFIndexOffset[0]=PanelIndexOffset[0]=0;
  for(int ns=1; ns<NumSurfaces; ns++)
   { BFIndexOffset[ns]=BFIndexOffset[ns-1] + Surfaces[ns-1]->NumBFs;
     PanelIndexOffset[ns]=PanelIndexOffset[ns-1] + Surfaces[ns-1]->NumPanels;
   };

  /***************************************************************/
  /* allocate space for cached epsilon and mu values *************/
  /***************************************************************/
  StoredOmega=0.0;
  EpsTF = (cdouble *)mallocEC(NumRegions * sizeof(cdouble));
  MuTF  = (cdouble *)mallocEC(NumRegions * sizeof(cdouble));

  /***************************************************************/
  /* initialize Mate[] array.                                    */
  /*                                                             */
  /* how it works:                                               */
  /*                                                             */
  /* (1) two surfaces are considered identical if                */
  /*     (a) they were read in from the same physical region of  */
  /*         the same mesh file, and                             */
  /*     (b) the two regions they bound have the same material   */
  /*         properties.                                         */
  /*                                                             */
  /* (2) Mate[] array: If surfaces i, j, k, ... are identical and*/
  /*                   i<j<k<..., then we set                    */
  /*                   Mate[i] = -1                              */
  /*                   Mate[j] = i                               */
  /*                   Mate[k] = i                               */
  /***************************************************************/
  Mate=(int *)mallocEC(NumSurfaces*sizeof(int));
  Mate[0]=-1;
  for(int ns=1; ns<NumSurfaces; ns++)
   { S=Surfaces[ns];
     Mate[ns]=-1;
     int nr1=S->RegionIndices[0];
     int nr2=S->RegionIndices[1];
     for(int nsp=0; nsp<ns && Mate[ns]==-1; nsp++)
      { SP=Surfaces[nsp];
        int nr1p=SP->RegionIndices[0];
        int nr2p=SP->RegionIndices[1];
        if (    ( !strcmp(S->MeshFileName, SP->MeshFileName)           )
             && ( S->PhysicalRegion == SP->PhysicalRegion              )
             && ( !strcmp(RegionMPs[nr1]->Name, RegionMPs[nr1p]->Name) )
             && (   (S->IsPEC && SP->IsPEC)
                 || (!S->IsPEC && !SP->IsPEC && !strcmp(RegionMPs[nr2]->Name, RegionMPs[nr2p]->Name) )
                )
           ) 
         Mate[ns]=nsp;
      };
   };

  /***************************************************************/
  /* initialize SurfaceMoved[] array.                            */
  /* the values of this array are only defined after             */
  /* a call to the RWGGeometry::Transform() function, when we    */
  /* have SurfaceMoved[i]=1 if the ith surface was modified by   */
  /* the transformation.                                         */
  /***************************************************************/
  SurfaceMoved=(int *)mallocEC(NumSurfaces*sizeof(int));

}

/***************************************************************/
/* RWGGeometry class destructor *******************************/
/***************************************************************/
RWGGeometry::~RWGGeometry()
{
  int ns;

  for(int ns=0; ns<NumSurfaces; ns++)
   delete Surfaces[ns];

  free(Surfaces);

  for(int nr=0; nr<NumRegions; nr++)
   { delete RegionMPs[nr];
     free(RegionLabels[nr]); 
   };
  free(RegionMPs);
  free(RegionLabels);

  free(BFIndexOffset);
  free(PanelIndexOffset);
  free(EpsTF);
  free(MuTF);
  free(Mate);
  free(SurfaceMoved);
  free(GeoFileName);

}

/***************************************************************/
/* return the index of the region whose label is Label. (This  */
/* index is between 0 and NumRegions-1, and is the index into  */
/* the RegionLabels[] and RegionMPs[] arrays for the object    */
/* in question.)                                               */
/*                                                             */
/* if there was no such region, -1 is returned.                */
/***************************************************************/
int RWGGeometry::GetRegionByLabel(const char *Label)
{
  if (Label==0)
   return -1;
  
  for(int nr=0; nr<NumRegions; nr++)
   if ( !strcasecmp(Label, RegionLabels[nr]) ) 
    return nr;

  return -1;
}

/***************************************************************/
/* return the RWGSurface whose label is Label. if pns is       */
/* non-NULL on entry, then on return it is set to the index of */
/* the RWGSurface. If no corresponding RWGSurface was found,   */
/* the return value is NULL and *pns is set to -1.             */
/***************************************************************/
RWGSurface *RWGGeometry::GetSurfaceByLabel(const char *Label, int *pns)
{
  if (pns) *pns=-1;
 
  if (Label==0)
   return NULL;
  
  for(int ns=0; ns<NumSurfaces; ns++)
   if ( !strcasecmp(Label, Surfaces[ns]->Label) ) 
    { if (pns) *pns = ns;
      return Surfaces[ns];
    }

  return NULL;
}

/***************************************************************/
/* Apply the specified GTComplex to transform the geometry.    */
/* (Note that a 'GTComplex' is a list of GTransformations, each*/
/* of which is applied to one specific surface in the geometry.)*/
/***************************************************************/
void RWGGeometry::Transform(GTComplex *GTC)
{ 
  int nsa, WhichSurface;
  RWGSurface *S;

  // assume that no objects will be modified by this operation
  memset(SurfaceMoved, 0, NumSurfaces*sizeof(int));

  // loop over the individual transformations in the complex
  for(nsa=0; nsa<GTC->NumSurfacesAffected; nsa++)
   { 
     // find the surface corresponding to the label for this transformation
     S=GetSurfaceByLabel(GTC->SurfaceLabel[nsa], &WhichSurface);

     // apply the transformation to that object
     if (S) 
      { S->Transform(GTC->GT + nsa);
        SurfaceMoved[WhichSurface]=1;
      };
        
   };

}

/***************************************************************/
/* Undo transformations. ***************************************/
/***************************************************************/
void RWGGeometry::UnTransform()
{ 
  for(int ns=0; ns<NumSurfaces; ns++)
   Surfaces[ns]->UnTransform();
}

/***************************************************************/
/* Quick sanity check to make sure that a given list of        */
/* GTComplex structures actually makes sense for the given     */
/* geometry, which is to say that it doesn't request           */
/* transformations on any surfaces that don't exist in the     */
/* geometry.                                                   */
/* Returns 0 if the check passed, or an error message if not.  */
/***************************************************************/
char *RWGGeometry::CheckGTCList(GTComplex **GTCList, int NumGTCs)
{
  int ngtc, nsa;
  
  for(ngtc=0; ngtc<NumGTCs; ngtc++)
   for (nsa=0; nsa<GTCList[ngtc]->NumSurfacesAffected; nsa++)
    if (!GetSurfaceByLabel(GTCList[ngtc]->SurfaceLabel[nsa]))
     return vstrdup("transformation requested for unknown surface %s",
                     GTCList[ngtc]->SurfaceLabel[nsa]);

  return 0;
}

/***************************************************************/
/* Return the dimension of the linear system. ******************/
/***************************************************************/
int RWGGeometry::GetDimension()
{ return TotalBFs; }

/***************************************************************/
/***************************************************************/
/***************************************************************/
void RWGGeometry::SetLogLevel(int NewLogLevel)
 { LogLevel=NewLogLevel; }

/***************************************************************/
/***************************************************************/
/***************************************************************/
void RWGGeometry::SetEpsMu(const char *Label, cdouble Eps, cdouble Mu)
{ 
  int nr=GetRegionByLabel(Label);
  if (nr==-1)
   Warn("unknown object %s specified in SetEpsMu() (ignoring)",Label);
  else
   RegionMPs[nr]->SetEpsMu(Eps, Mu);
} 

void RWGGeometry::SetEpsMu(cdouble Eps, cdouble Mu)
{ SetEpsMu(0, Eps, Mu); }

void RWGGeometry::SetEps(const char *Label, cdouble Eps)
{ SetEpsMu(Label, Eps, 1.0); }

void RWGGeometry::SetEps(cdouble Eps)
{ SetEpsMu(0, Eps, 1.0); }

/***************************************************************/
/* update the internally stored cache of epsilon and mu values */
/* for a given frequency                                       */
/***************************************************************/
void RWGGeometry::UpdateCachedEpsMuValues(cdouble Omega)
{
  /*--------------------------------------------------------------*/
  /*- update cached epsilon and mu values at this frequency if    */
  /*- necessary                                                   */
  /*--------------------------------------------------------------*/
  if (Omega != StoredOmega )
   { StoredOmega=Omega;
     for(int nr=0; nr<NumRegions; nr++)
      RegionMPs[nr]->GetEpsMu(Omega, &(EpsTF[nr]), &(MuTF[nr]) );
   };
}

} // namespace scuff
