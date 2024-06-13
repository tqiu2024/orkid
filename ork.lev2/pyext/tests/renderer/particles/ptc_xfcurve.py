#!/usr/bin/env python3

################################################################################
# lev2 sample which renders a scenegraph to a window
# Copyright 1996-2023, Michael T. Mayers.
# Distributed under the MIT License
# see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
################################################################################

import math, sys, os, random, numpy, argparse
from obt import path
from pathlib import Path
from orkengine.core import *
from orkengine.lev2 import *
lev2_pyexdir.addToSysPath()
from lev2utils.cameras import *
from lev2utils.scenegraph import createSceneGraph

tokens = CrcStringProxy()

from _ptc_harness import *

################################################################################
parser = argparse.ArgumentParser(description='scenegraph particles example')

args = vars(parser.parse_args())

################################################################################

class ParticlesApp(object):

  def __init__(self):
    super().__init__()
    self.ezapp = OrkEzApp.create(self)
    self.ezapp.setRefreshPolicy(RefreshFastest, 0)

    #self.materials = set()

    setupUiCamera( app=self, #
                   eye = vec3(0,0,30), #
                   constrainZ=True, #
                   up=vec3(0,1,0))

  ################################################
  # gpu data init:
  #  called on main thread when graphics context is
  #   made available
  ##############################################

  def onGpuInit(self,ctx):

    ###################################
    # create scenegraph
    ###################################

    createSceneGraph(app=self,rendermodel="ForwardPBR")

    ###################################
    # create particle drawable 
    ###################################

    ptc_data = {
      "GLOB":particles.Globals,
      "POOL":particles.Pool,
      "EMITN":particles.NozzleEmitter,
      "EMITR":particles.RingEmitter,
      "GLOB":particles.Globals,
      "GRAV":particles.Gravity,
      "TURB":particles.Turbulence,
      "VORT":particles.Vortex,
      "SPRI":particles.SpriteRenderer,
    }
    ptc_connections = [
      ("POOL","EMITN"),
      ("EMITN","EMITR"),
      ("EMITR","GRAV"),
      ("GRAV","TURB"),
      ("TURB","VORT"),
      ("VORT","SPRI"),
    ]
    createParticleData(self,ptc_data,ptc_connections,self.layer1)

    print(self.GLOB)
    #assert(False)

    self.POOL.pool_size = 16384 # max number of particles in pool

    self.SPRI.inputs.Size = 0.05
    self.SPRI.inputs.GradientIntensity = 1
    self.SPRI.material = presetMaterial()
    self.EMITN.inputs.EmissionVelocity = 0.1
    presetPOOL1(self.POOL)
    presetEMITN1(self.EMITN)
    presetEMITR1(self.EMITR)
    presetTURB1(self.TURB)
    presetVORT1(self.VORT)
    presetGRAV1(self.GRAV)
    
    
    self.TURB.inputs.Amount = vec3(1,1,1)*5

    xf_size_curve = dataflow.floatxf.multicurve()
    xf_size_curve.multicurve.splitSegment(0)
    xf_size_curve.multicurve.setPoint(0,0,0)
    xf_size_curve.multicurve.setPoint(1,0.5,0.25)
    xf_size_curve.multicurve.setPoint(2,1,0)
        

    self.SPRI.inputs.Size.transformer.append(dataflow.floatxf.mod(1.0))
    self.SPRI.inputs.Size.transformer.append(xf_size_curve)
    self.SPRI.inputs.Size.transformer.append(dataflow.floatxf.bias(0.25))
    self.SPRI.inputs.Size.transformer.append(dataflow.floatxf.sine())
    
    self.graphdata.connect(self.SPRI.inputs.Size,#
                           self.GLOB.outputs.RelTime)

    ##################
    # create particle sg node
    ##################



  ################################################

  def onUpdate(self,updinfo):
    self.scene.updateScene(self.cameralut) # update and enqueue all scenenodes

  ##############################################

  def onUiEvent(self,uievent):
    handled = self.uicam.uiEventHandler(uievent)
    if handled:
      self.camera.copyFrom( self.uicam.cameradata )
    return ui.HandlerResult()
###############################################################################

ParticlesApp().ezapp.mainThreadLoop()
