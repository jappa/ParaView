/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRConnectivity.h

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
// .NAME vtkAMRConnectivity - Identify fragments in the grid
//
// .SECTION Description
//
// .SEE vtkAMRConnectivity

#ifndef __vtkAMRConnectivity_h
#define __vtkAMRConnectivity_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkMultiBlockDataSetAlgorithm.h" 
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
#include <string>  // STL required.
#include <vector>  // STL required.

class vtkNonOverlappingAMR;
class vtkUniformGrid;
class vtkIdTypeArray;
class vtkIntArray;
class vtkAMRDualGridHelper;
class vtkAMRDualGridHelperBlock; 
class vtkAMRConnectivityEquivalence;
class vtkMPIController;
class vtkUnsignedCharArray;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkAMRConnectivity : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkAMRConnectivity,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkAMRConnectivity *New();

  // Description:
  // Add to list of volume arrays to find connected fragments
  void AddInputVolumeArrayToProcess(const char* name);
  void ClearInputVolumeArrayToProcess();

  // Description:
  // Get / Set volume fraction value.
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  vtkSetMacro(VolumeFractionSurfaceValue, double);

  // Description:
  // Get / Set where to resolve the regions between blocks
  vtkGetMacro(ResolveBlocks, bool);
  vtkSetMacro(ResolveBlocks, bool);

  // Description:
  // Get / Set where to resolve the regions between blocks
  vtkGetMacro(PropagateGhosts, bool);
  vtkSetMacro(PropagateGhosts, bool);

protected:
  vtkAMRConnectivity();
  ~vtkAMRConnectivity();

  double VolumeFractionSurfaceValue;
  vtkAMRDualGridHelper* Helper;
  vtkAMRConnectivityEquivalence* Equivalence;
  
  bool ResolveBlocks;
  bool PropagateGhosts;

  std::string RegionName;
  vtkIdType NextRegionId;

  // BTX
  std::vector<std::string> VolumeArrays;

  std::vector<std::vector <vtkSmartPointer<vtkIdTypeArray> > > BoundaryArrays;
  std::vector<std::vector <int> > ReceiveList;

  std::vector<bool> ValidNeighbor;
  std::vector<std::vector <std::vector <int> > > NeighborList;
  std::vector< vtkSmartPointer<vtkIntArray> > EquivPairs;

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*);

  int DoRequestData (vtkNonOverlappingAMR*, const char*);
  int WavePropagation (vtkIdType cellIdStart, 
                       vtkUniformGrid* grid, 
                       vtkIdTypeArray* regionId,
                       vtkDataArray* volArray,
                       vtkUnsignedCharArray* ghostArray);

  vtkAMRDualGridHelperBlock* GetBlockNeighbor (
                       vtkAMRDualGridHelperBlock* block, 
                       int dir);
  void ProcessBoundaryAtBlock (vtkNonOverlappingAMR* volume,
                               vtkAMRDualGridHelperBlock* block, 
                               vtkAMRDualGridHelperBlock* neighbor, 
                               int dir);
  int ExchangeBoundaries (vtkMPIController* controller);
  int ExchangeEquivPairs (vtkMPIController* controller);
  void ProcessBoundaryAtNeighbor (vtkNonOverlappingAMR* volume,
                                  vtkIdTypeArray *array);

private:
  vtkAMRConnectivity(const vtkAMRConnectivity&);  // Not implemented.
  void operator=(const vtkAMRConnectivity&);  // Not implemented.
  // ETX
};

#endif /* __vtkAMRConnectivity_h */
