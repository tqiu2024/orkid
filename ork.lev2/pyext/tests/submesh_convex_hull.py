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
  ##############################################
  def onGpuInit(self,ctx):
    super().onGpuInit(ctx,add_grid=False)
    ##############################
    self.pseudowire_pipe = self.createPseudoWirePipeline()
    solid_wire_pipeline = self.createBaryWirePipeline()
    material = solid_wire_pipeline.sharedMaterial
    solid_wire_pipeline.bindParam( material.param("m"), tokens.RCFD_M)
    ##############################
    self.fpmtx1 = mtx4.perspective(45*constants.DTOR,1,0.3,5)
    self.fvmtx1 = mtx4.lookAt(vec3(0,0,1),vec3(0,0,0),vec3(0,1,0))
    self.frustum1 = Frustum()
    self.frustum1.set(self.fvmtx1,self.fpmtx1)
    self.frusmesh1 = meshutil.SubMesh.createFromFrustum(self.frustum1,projective_rect_uv=True)
    self.submesh1 = strippedSubmesh(self.frusmesh1)
    self.prim1 = meshutil.RigidPrimitive(self.frusmesh1,ctx)
    self.sgnode1 = self.prim1.createNode("m1",self.layer1,self.pseudowire_pipe)
    self.sgnode1.enabled = True
    self.sgnode1.sortkey = 2;
    self.sgnode1.modcolor = vec4(1,0,0,1)
    ##############################
    self.fpmtx2 = mtx4.perspective(45*constants.DTOR,1,0.3,5)
    self.fvmtx2 = mtx4.lookAt(vec3(1,0,1),vec3(1,1,0),vec3(0,1,0))
    self.frustum2 = Frustum()
    self.frustum2.set(self.fvmtx2,self.fpmtx2)
    self.frusmesh2 = meshutil.SubMesh.createFromFrustum(self.frustum2,projective_rect_uv=True)
    self.submesh2 = strippedSubmesh(self.frusmesh2)
    self.prim2 = meshutil.RigidPrimitive(self.frusmesh2,ctx)
    self.sgnode2 = self.prim2.createNode("m2",self.layer1,self.pseudowire_pipe)
    self.sgnode2.enabled = True
    self.sgnode2.sortkey = 2;
    self.sgnode2.modcolor = vec4(0,1,0,1)
    ##############################
    submesh_isect = proc_with_frustum(self.submesh1,self.frustum2)
    self.barysub_isect = submesh_isect.withBarycentricUVs()
    self.prim3 = meshutil.RigidPrimitive(self.barysub_isect,ctx)
    self.sgnode3 = self.prim3.createNode("m3",self.layer1,solid_wire_pipeline)
    self.sgnode3.enabled = True
    ################################################################################
    class UpdateSettings:
      def __init__(self):
        self.fov_min = 45
        self.fov_max = 135
        self.fov_speed = 1.8
        self.lat_min = -2
        self.lat_max = 2
        self.lat_speed = 1.0
        self.timeaccum = 0.0
        self.lat_accum = 0.0
        self.fov_accum = 0.0
      def computeFOV(self):
        θ = self.fov_accum
        deg = self.fov_min+math.sin(θ)*(self.fov_max-self.fov_min)
        return deg*constants.DTOR
      def computeLAT(self):
        θ = self.lat_accum
        return self.lat_min+math.sin(θ)*(self.lat_max-self.lat_min)
      def lerp(self,oth,alpha, deltatime):
        self.timeaccum += deltatime
        self.lat_accum += deltatime * self.lat_speed
        self.fov_accum += deltatime * self.fov_speed
        inv_alpha = 1.0-alpha
        self.fov_min   = self.fov_min*inv_alpha   + oth.fov_min*alpha
        self.fov_max   = self.fov_max*inv_alpha   + oth.fov_max*alpha
        self.fov_speed = self.fov_speed*inv_alpha + oth.fov_speed*alpha
        self.lat_min   = self.lat_min*inv_alpha   + oth.lat_min*alpha
        self.lat_max   = self.lat_max*inv_alpha   + oth.lat_max*alpha
        self.lat_speed = self.lat_speed*inv_alpha + oth.lat_speed*alpha
    ################################################################################
    self.upd_1a = UpdateSettings()
    self.upd_2a = UpdateSettings()
    self.upd_1b = UpdateSettings()
    self.upd_2b = UpdateSettings()
    self.upd_1c = UpdateSettings()
    self.upd_2c = UpdateSettings()
    ################################################################################
    self.upd_1a.fov_speed = 1.7
    self.upd_1a.fov_min = 45
    self.upd_1a.fov_max = 45
    self.upd_1a.lat_speed = 1.0
    ################################################################################
    self.upd_2a.fov_speed = 1.9
    self.upd_2a.fov_min = 45
    self.upd_2a.fov_max = 45
    self.upd_2a.lat_speed = 0.7
    ################################################################################
    self.upd_1b.fov_min = 45
    self.upd_1b.fov_max = 90
    self.upd_1b.fov_speed = 1.3
    ################################################################################
    self.upd_2b.fov_min = 45
    self.upd_2b.fov_max = 90
    self.upd_2b.fov_speed = 0.7
    ################################################################################
    self.upd_1c.fov_min = 150
    self.upd_1c.fov_max = 150
    self.upd_1c.fov_speed = 0
    self.upd_1c.lat_min = 0
    self.upd_1c.lat_max = 0
    ################################################################################
    self.upd_2c.fov_min = 150
    self.upd_2c.fov_max = 150
    self.upd_2c.fov_speed = 0
    self.upd_2c.lat_min = 0
    self.upd_2c.lat_max = 0
    ################################################################################
    self.upd_c1 = UpdateSettings()
    self.upd_c2 = UpdateSettings()
    self.dice = 2
    self.counter = 5
  ##############################################
  def onUpdate(self,updevent):
    super().onUpdate(updevent)
    ##############################
    # handle counter
    ##############################
    self.counter -= updevent.deltatime
    if self.counter<0:
      self.counter = random.uniform(2,5)
      old_dice = self.dice
      while self.dice==old_dice:
        self.dice = random.randint(0,2)
    ##############################
    lerp_rate = 0.01
    #self.dice = 0
    if self.dice==0:
      self.upd_c1.lerp(self.upd_1a,lerp_rate,updevent.deltatime)
      self.upd_c2.lerp(self.upd_2a,lerp_rate,updevent.deltatime)
    elif self.dice==1:
      self.upd_c1.lerp(self.upd_1b,lerp_rate,updevent.deltatime)
      self.upd_c2.lerp(self.upd_2b,lerp_rate,updevent.deltatime)
    elif self.dice==2:
      self.upd_c1.lerp(self.upd_1c,lerp_rate,updevent.deltatime)
      self.upd_c2.lerp(self.upd_2c,lerp_rate,updevent.deltatime)
    ##############################

    ##############################
    θ = self.abstime # * math.pi * 2.0 * 0.1
    #
    self.fpmtx1 = mtx4.perspective(self.upd_c1.computeFOV(),1,0.3,5)
    self.fpmtx2 = mtx4.perspective(self.upd_c2.computeFOV(),1,0.3,5)
    #2
    lat_1 = self.upd_c1.computeLAT()
    lat_2 = self.upd_c2.computeLAT()
    PLANAR_BIAS = 0.0016
    self.fvmtx1 = mtx4.lookAt(vec3(0,0,1),vec3(lat_1,0,0),vec3(0,1,0))
    self.fvmtx2 = mtx4.lookAt(vec3(1,0,1+PLANAR_BIAS),vec3(1,lat_2,PLANAR_BIAS),vec3(0,1,0))
    #
    self.frustum1.set(self.fvmtx1,self.fpmtx1)
    self.frustum2.set(self.fvmtx2,self.fpmtx2)
    #
    self.frusmesh1 = meshutil.SubMesh.createFromFrustum(self.frustum1,projective_rect_uv=True)
    self.frusmesh2 = meshutil.SubMesh.createFromFrustum(self.frustum2,projective_rect_uv=True)
    #
    submesh1 = strippedSubmesh(self.frusmesh1)
    submesh2 = strippedSubmesh(self.frusmesh2)

    isec1 = strippedSubmesh(submesh1)
    isec1.mergeSubmesh(strippedSubmesh(submesh2))
    self.submesh_isect = isec1.convexHull() #.coplanarJoined().triangulated()

    #time.sleep(0.25)
  ##############################################
  def onGpuIter(self):
    super().onGpuIter()

    #self.mutex.acquire()

    # two wireframe frustums
    self.prim1.fromSubMesh(self.frusmesh1,self.context)
    self.prim2.fromSubMesh(self.frusmesh2,self.context)
    # intersection mesh
    self.barysub_isect = self.submesh_isect.withBarycentricUVs()
    self.prim3.fromSubMesh(self.barysub_isect,self.context)
    #print("intersection convexVolume: %s" % submesh_isect.convexVolume)
    #self.mutex.release()

###############################################################################

sgapp = SceneGraphApp()

sgapp.ezapp.mainThreadLoop(on_iter=lambda: sgapp.onGpuIter() )

