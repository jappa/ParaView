/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCubeAxesDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCubeAxesDisplayProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCubeAxesActor2D.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPart.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMCubeAxesDisplayProxy);
vtkCxxRevisionMacro(vtkSMCubeAxesDisplayProxy, "1.13");


//----------------------------------------------------------------------------
vtkSMCubeAxesDisplayProxy::vtkSMCubeAxesDisplayProxy()
{
  this->Visibility = 1;
  this->GeometryIsValid = 0;
  this->Input = 0;
  this->OutputPort = 0;
  this->Caches = 0;
  this->NumberOfCaches = 0;

  this->CubeAxesProxy = 0;
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMCubeAxesDisplayProxy::~vtkSMCubeAxesDisplayProxy()
{
  this->CubeAxesProxy = 0;
  
  // No reference counting for this ivar.
  this->Input = 0;
  this->RemoveAllCaches();
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->CubeAxesProxy = this->GetSubProxy("Prop2D");
  if (!this->CubeAxesProxy)
    {
    vtkErrorMacro("SubProxy CubeAxes must be defined.");
    return;
    }
  
  this->CubeAxesProxy->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
 
  this->Superclass::CreateVTKObjects();

  vtkSMIntVectorProperty* ivp;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CubeAxesProxy->GetProperty("FlyMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property FlyMode.");
    return;
    }
  ivp->SetElement(0, 0); // FlyToOuterEdges.
  

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CubeAxesProxy->GetProperty("Inertia"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Inertia.");
    return;
    }
  ivp->SetElement(0, 20);

  this->CubeAxesProxy->UpdateVTKObjects(); 
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::AddInput(unsigned int,
                                         vtkSMSourceProxy* input,
                                         unsigned int outputPort,
                                         const char*)
{
  this->SetInput(input);
  this->OutputPort = outputPort;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::SetInput(vtkSMProxy* input)
{  
  this->CreateVTKObjects();
  //input->AddConsumer(0, this); This will happen automatically when
  //the caller uses ProxyProperty to add the input.
  
  // Hang onto the input since cube axes bounds are set manually.
  this->Input = vtkSMSourceProxy::SafeDownCast(input);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  if (!rm)
    {
    return;
    }  
  if (this->RenderModuleProxy)
    {
    vtkErrorMacro("Can be added only to one render module.");
    return;
    }
  this->Superclass::AddToRenderModule(rm);

  // We don't set the Camera proxy for the cube axes actor using 
  // properties since the Camera Proxy provided by the RenderModule is only 
  // on the CLIENT, and CubeAxesActor needs the camera on the servers as well.
  vtkClientServerStream stream;
  vtkSMProxy* renderer = this->GetRenderer2DProxy(rm);
  stream << vtkClientServerStream::Invoke
         << renderer->GetID()
         << "GetActiveCamera" << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->CubeAxesProxy->GetID()
         << "SetCamera" << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->CubeAxesProxy->GetServers(), stream);
  this->RenderModuleProxy = rm;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::RemoveFromRenderModule(
  vtkSMRenderModuleProxy* rm)
{
  if (!rm || this->RenderModuleProxy != rm)
    {
    return;
    }
  this->Superclass::AddToRenderModule(rm);

  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->CubeAxesProxy->GetProperty("Camera"));
  pp->RemoveAllProxies();
  this->CubeAxesProxy->UpdateVTKObjects(); 
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::SetVisibility(int v)
{
  if (v)
    {
    v = 1;
    }
  if (v == this->Visibility)
    {
    return;
    }    
  this->GeometryIsValid = 0;  // so we can change the color
  this->Visibility = v;
  
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CubeAxesProxy->GetProperty("Visibility"));
  ivp->SetElement(0, v);
  this->CubeAxesProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  this->Superclass::MarkModified(modifiedProxy);
  this->InvalidateGeometry();
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::InvalidateGeometryInternal(int /*useCache*/)
{
  this->GeometryIsValid = 0;
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::Update(vtkSMAbstractViewModuleProxy*)
{
  if (this->GeometryIsValid || !this->RenderModuleProxy)
    {
    return;
    }
    
  double bounds[6];
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  
  vtkClientServerStream stream;
  
  double rgb[3];
  double *background;
  rgb[0] = rgb[1] = rgb[2] = 1.0;

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("Background"));
  if (!dvp)
    {
    background = rgb;
    }
  background = dvp->GetElements();
  
  // Change the color of the cube axes if the background is light.
  if (background[0] + background[1] + background[2] > 2.2)
    {
    rgb[0] = rgb[1] = rgb[2] = 0.0;
    }

  if (this->Input == 0)
    {
    return;
    }

  this->Input->UpdatePipeline();    
  vtkPVDataInformation* dataInfo = this->Input->GetDataInformation(
    this->OutputPort);
  dataInfo->GetBounds(bounds);
  stream << vtkClientServerStream::Invoke 
         << this->CubeAxesProxy->GetID() << "SetBounds"
         << bounds[0] << bounds[1] << bounds[2]
         << bounds[3] << bounds[4] << bounds[5]
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->CubeAxesProxy->GetID() << "GetProperty"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult << "SetColor"
         << rgb[0] << rgb[1] << rgb[2]
         << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke
         << this->CubeAxesProxy->GetID() << "GetAxisTitleTextProperty"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult << "SetColor"
         << rgb[0] << rgb[1] << rgb[2]
         << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke
         << this->CubeAxesProxy->GetID() << "GetAxisLabelTextProperty"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult << "SetColor"
         << rgb[0] << rgb[1] << rgb[2]
         << vtkClientServerStream::End;  
  pm->SendStream(this->ConnectionID, this->CubeAxesProxy->GetServers(), stream);
  this->GeometryIsValid = 1;

  this->InvokeEvent(vtkSMAbstractDisplayProxy::ForceUpdateEvent);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::RemoveAllCaches()
{
  if (this->NumberOfCaches == 0)
    {
    return;
    }
  int i;
  for (i = 0; i < this->NumberOfCaches; ++i)
    {
    if (this->Caches[i])
      {
      delete [] this->Caches[i];
      this->Caches[i] = 0;
      }
    }
  delete [] this->Caches;
  this->Caches = 0;
  this->NumberOfCaches = 0;
}

//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkSMCubeAxesDisplayProxy::CacheUpdate(int idx, int total)
{
  int i;
  if (total != this->NumberOfCaches)
    {
    this->RemoveAllCaches();
    this->Caches = new double*[total];
    for (i = 0; i < total; ++i)
      {
      this->Caches[i] = 0;
      }
    this->NumberOfCaches = total;
    }

  if (this->Caches[idx] == 0)
    {
    this->Input->UpdatePipeline();
    vtkPVDataInformation* info = this->Input->GetDataInformation(
      this->OutputPort);
    this->Caches[idx] = new double[6];
    info->GetBounds(this->Caches[idx]);
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream; 
  stream << vtkClientServerStream::Invoke 
         << this->CubeAxesProxy->GetID() << "SetBounds"
         << this->Caches[idx][0] << this->Caches[idx][1] 
         << this->Caches[idx][2] << this->Caches[idx][3] 
         << this->Caches[idx][4] << this->Caches[idx][5]
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->CubeAxesProxy->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "CubeAxesProxy: " << this->CubeAxesProxy << endl;
}

