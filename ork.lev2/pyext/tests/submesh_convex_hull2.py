#!/usr/bin/env python3
################################################################################
# Copyright 1996-2023, Michael T. Mayers.
# Distributed under the MIT License
# see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
################################################################################
import math, random, argparse, sys, time
from threading import Lock
from orkengine.core import *
from orkengine.lev2 import *
from _boilerplate import *
import math, sys, os, random, numpy
################################################################################

def strippedSubmesh(inpsubmesh):
  stripped = inpsubmesh.copy(preserve_normals=False,
                             preserve_colors=False,
                             preserve_texcoords=False)
  return stripped

################################################################################

def proc_with_plane(inpsubmesh,plane):
  submesh2 = strippedSubmesh(inpsubmesh).clippedWithPlane(plane=plane,
                                         close_mesh=True, 
                                         flip_orientation=False )["front"]

  return submesh2 

################################################################################

def proc_with_frustum(inpsubmesh,frustum):
  submesh2 = proc_with_plane(inpsubmesh,frustum.nearPlane)
  submesh3 = proc_with_plane(submesh2,frustum.farPlane)
  submesh4 = proc_with_plane(submesh3,frustum.leftPlane)
  submesh5 = proc_with_plane(submesh4,frustum.rightPlane)
  submesh6 = proc_with_plane(submesh5,frustum.topPlane)
  submesh7 = proc_with_plane(submesh6,frustum.bottomPlane)
  return submesh7

################################################################################

class SceneGraphApp(BasicUiCamSgApp):

  ##############################################
  def __init__(self):
    super().__init__()
    self.mutex = Lock()
    self.uicam.lookAt( vec3(0,0,20), vec3(0,0,0), vec3(0,1,0) )
    self.camera.copyFrom( self.uicam.cameradata )
    self.NUMPOINTS = 16
    self.pnt = [vec3(0) for i in range(self.NUMPOINTS)]
  ##############################################
  def updatePoints(self,abstime):
    paramA = 4+math.sin(abstime*0.2)*4
    paramB = 1+math.sin(abstime*0.23)*0.15
    paramC = 1+math.sin(abstime*0.27)*0.05
    paramD = 1+math.sin(abstime*0.29)*0.025

    for i in range(self.NUMPOINTS):
        t = i/(self.NUMPOINTS-1)
        t = t*2*math.pi
        r = 1.0
        # compute sphereical coordinates
        x = r*math.sin(t*paramA)*math.cos(t*paramB)
        y = r*math.sin(t*paramA)*math.sin(t*paramB)
        z = r*math.cos(t*paramA)
        self.pnt[i] = vec3(x,y,z)*4

  ##############################################
  def onGpuInit(self,ctx):
    super().onGpuInit(ctx,add_grid=False)
    ##############################
    self.pseudowire_pipe = self.createPseudoWirePipeline()
    solid_wire_pipeline = self.createBaryWirePipeline()
    material = solid_wire_pipeline.sharedMaterial
    solid_wire_pipeline.bindParam( material.param("m"), tokens.RCFD_M)
    ##############################
    submesh_isect = meshutil.SubMesh()
    self.barysub_isect = submesh_isect.withBarycentricUVs()
    self.prim3 = meshutil.RigidPrimitive(self.barysub_isect,ctx)
    self.sgnode3 = self.prim3.createNode("m3",self.layer1,solid_wire_pipeline)
    self.sgnode3.enabled = True
    ################################################################################
  ##############################################
  def onUpdate(self,updevent):
    super().onUpdate(updevent)
    self.updatePoints(updevent.absolutetime)
    θ = self.abstime # * math.pi * 2.0 * 0.1
    ##############################
    submesh_isect = meshutil.SubMesh()
    for i in range(self.NUMPOINTS):
      submesh_isect.makeVertex(position=self.pnt[i])
    self.barysub_isect = submesh_isect.convexHull().withBarycentricUVs()
    ##############################

    #time.sleep(0.25)
  ##############################################
  def onGpuIter(self):
    super().onGpuIter()

    # intersection mesh
    #self.barysub_isect = self.submesh_isect.withBarycentricUVs()
    self.prim3.fromSubMesh(self.barysub_isect,self.context)

###############################################################################

sgapp = SceneGraphApp()

sgapp.ezapp.mainThreadLoop(on_iter=lambda: sgapp.onGpuIter() )

