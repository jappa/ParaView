/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeProp.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeProp.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"

vtkStandardNewMacro(vtkCompositeProp);

vtkCxxRevisionMacro(vtkCompositeProp, "1.4");

// Creates an Prop with the following defaults: visibility on.
vtkCompositeProp::vtkCompositeProp()
{
  this->Props = vtkPropCollection::New();

  vtkMath::UninitializeBounds(this->Bounds);
}

vtkCompositeProp::~vtkCompositeProp()
{
  this->Props->Delete();
}

#define vtkMAX(a,b) (((a)>(b))?(a):(b))
#define vtkMIN(a,b) (((a)<(b))?(a):(b))

double *vtkCompositeProp::GetBounds()
{
  // Calculate bounds
  int cc;
  for (cc =0; cc < 3; cc ++ )
    {
    this->Bounds[cc*2] = VTK_DOUBLE_MAX;
    this->Bounds[cc*2+1] = -VTK_DOUBLE_MAX;    
    }

  int hasBounds = 0;
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    double *tb = p->GetBounds();
    if ( tb )
      {
      hasBounds = 1;
      for ( cc = 0; cc < 3; cc ++ )
        {       
        this->Bounds[cc*2] = vtkMIN(this->Bounds[cc*2], tb[cc*2]);
        this->Bounds[cc*2+1] = vtkMAX(this->Bounds[cc*2+1], tb[cc*2+1]);
        }
      }
    }

  if (!hasBounds)
    {
    return 0;
    }

  return this->Bounds;
}

// This method is invoked if the prop is picked.
// This method is invoked if the prop is picked.
void vtkCompositeProp::Pick()
{
  this->Superclass::Pick();
}

int vtkCompositeProp::RenderOpaqueGeometry(vtkViewport *v)
{
  int renderCount = 0;
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    renderCount += p->RenderOpaqueGeometry(v);
    }
  return renderCount;
}

int vtkCompositeProp::RenderTranslucentGeometry(vtkViewport *v)
{
  int renderCount = 0;
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    renderCount += p->RenderTranslucentGeometry(v);
    }
  return renderCount;
}

int vtkCompositeProp::RenderOverlay(vtkViewport *v)
{
  int renderCount = 0;
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    renderCount += p->RenderOverlay(v);
    }
  return renderCount;
}

void vtkCompositeProp::ReleaseGraphicsResources(vtkWindow *v)
{
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    p->ReleaseGraphicsResources(v);
    }
}

void vtkCompositeProp::AddProp(vtkProp *p)
{
  this->Props->AddItem(p);
}

void vtkCompositeProp::RemoveProp(vtkProp *p)
{
  this->Props->RemoveItem(p);
}

void vtkCompositeProp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Props: " << this->Props << endl;
}

