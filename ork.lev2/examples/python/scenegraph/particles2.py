#!/usr/bin/env python3

################################################################################
# lev2 sample which renders a scenegraph to a window
# Copyright 1996-2020, Michael T. Mayers.
# Distributed under the Boost Software License - Version 1.0 - August 17, 2003
# see http://www.boost.org/LICENSE_1_0.txt
################################################################################

import math, sys, os, random, numpy, argparse
from pathlib import Path
from orkengine.core import *
from orkengine.lev2 import *
sys.path.append((thisdir()/"..").normalized.as_string) # add parent dir to path
from common.cameras import *
from common.primitives import createParticleData
from common.scenegraph import createSceneGraph

################################################################################
parser = argparse.ArgumentParser(description='scenegraph particles example')
parser.add_argument('--dynaplugs', action="store_true", help='dynamic plug update' )

args = vars(parser.parse_args())
dynaplugs = args["dynaplugs"]

################################################################################

class ParticlesApp(object):

  def __init__(self):
    super().__init__()
    self.ezapp = OrkEzApp.create(self)
    self.ezapp.setRefreshPolicy(RefreshFastest, 0)
    self.materials = set()
    setupUiCamera( app=self, eye = vec3(0,0,30), constrainZ=True, up=vec3(0,1,0))

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

    self.ptc_data = createParticleData()
    ptc_drawable = self.ptc_data.drawable_data.createDrawable()

    self.emitterplugs = self.ptc_data.emitter.inputs
    self.emitter2plugs = self.ptc_data.emitter2.inputs
    self.vortexplugs = self.ptc_data.vortex.inputs
    self.gravityplugs = self.ptc_data.gravity.inputs
    self.turbulenceplugs = self.ptc_data.turbulence.inputs

    self.emitterplugs.LifeSpan = 20
    self.emitterplugs.EmissionRate = 0
    self.emitterplugs.DispersionAngle = 0
    emitter_pos = vec3()
    emitter_pos.x = 0
    emitter_pos.y = 2
    emitter_pos.z = 0
    self.emitterplugs.Offset = emitter_pos

    ##################
    # create particle sg node
    ##################

    self.particlenode = self.layer1.createDrawableNode("particle-node",ptc_drawable)
    self.particlenode.sortkey = 2;

  ################################################
  def configA(self,abstime):
    self.emitter2plugs.EmitterSpinRate = math.sin(abstime*0.25)*10
    self.emitter2plugs.LifeSpan = 5
    self.vortexplugs.VortexStrength = 2
    self.vortexplugs.OutwardStrength = -2
    self.vortexplugs.Falloff = 0
    self.gravityplugs.G = .5
    self.turbulenceplugs.Amount = vec3(0,0,0)

  ################################################
  def configB(self,abstime):
    self.emitter2plugs.EmitterSpinRate = math.sin(abstime*0.25)*1
    self.emitter2plugs.LifeSpan = 2
    self.vortexplugs.VortexStrength = .5
    self.vortexplugs.OutwardStrength = -.5
    self.vortexplugs.Falloff = 0
    self.gravityplugs.G = 1
    self.turbulenceplugs.Amount = vec3(2,2,2)

  ################################################
  def configC(self,abstime):
    self.emitter2plugs.EmitterSpinRate = math.sin(abstime)*30
    self.emitter2plugs.LifeSpan = 5
    self.vortexplugs.VortexStrength = 0
    self.vortexplugs.OutwardStrength = 1
    self.vortexplugs.Falloff = 0
    self.gravityplugs.G = 1.1
    self.turbulenceplugs.Amount = vec3(8,8,8)

  ################################################

  def onUpdate(self,updinfo):
    abstime = updinfo.absolutetime
    INDEX = int(math.fmod(abstime,24)/8)
    if(INDEX==0):
        self.configA(abstime)
    elif(INDEX==1):
        self.configB(abstime)
    else:
        self.configC(abstime)

    ########################################
    self.scene.updateScene(self.cameralut) # update and enqueue all scenenodes
    ########################################

  ##############################################

  def onUiEvent(self,uievent):
    handled = self.uicam.uiEventHandler(uievent)
    if handled:
      self.camera.copyFrom( self.uicam.cameradata )

###############################################################################

ParticlesApp().ezapp.mainThreadLoop()
