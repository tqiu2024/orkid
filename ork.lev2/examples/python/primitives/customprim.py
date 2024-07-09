#!/usr/bin/env python3
################################################################################
# lev2 sample which renders to an offscreen buffer
# Copyright 1996-2023, Michael T. Mayers.
# Distributed under the MIT License
# see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
################################################################################

#pip3 install numpi Pillow

import numpy, time, os, sys
import ork.path
from orkengine.core import *
from orkengine.lev2 import *
from ork.command import run
from PIL import Image
#########################################
# Our intention is not to 'install' anything just for running the examples
#  so we will just hack the sys,path
#########################################
from pathlib import Path
this_dir = Path(os.path.dirname(os.path.abspath(__file__)))
pyex_dir = (this_dir/"..").resolve()
sys.path.append(str(pyex_dir))
from lev2utils.shaders import Shader
from lev2utils.submeshes import FrustumQuads
#########################################
os.chdir(os.environ["ORKID_WORKSPACE_DIR"])
################################################################################
# set up image dimensions, with antialiasing
################################################################################
ANTIALIASDIM = 4
WIDTH = 1080
HEIGHT = 640
AAWIDTH = WIDTH*ANTIALIASDIM
AAHEIGHT = HEIGHT*ANTIALIASDIM
###################################
# setup context, shaders
###################################
lev2appinit()
gfxenv = GfxEnv.ref
ctx = gfxenv.loadingContext()
FBI = ctx.FBI()
GBI = ctx.GBI()
print(ctx)
ctx.makeCurrent()
FontManager.gpuInit(ctx)
sh = Shader(ctx)
###################################
# get submesh
###################################
inp_submesh = FrustumQuads()
###################################
# generate primitive
###################################
tsubmesh = inp_submesh.triangulate()
tsubmesh.writeWavefrontObj("customprim.obj")
prim = RigidPrimitive(tsubmesh,ctx)
###################################
# rtg setup
###################################
rtg = ctx.defaultRTG()
rtb = rtg.buffer(0)
ctx.resize(AAWIDTH,AAHEIGHT)
capbuf = CaptureBuffer()

texture = Texture.load("lev2://textures/voltex_pn3")
print(texture)
lev2apppoll() # process opq
###################################
# setup camera
###################################
pmatrix = ctx.perspective(45,WIDTH/HEIGHT,0.01,100.0)
vmatrix = ctx.lookAt(vec3(-5,3,1),
                     vec3(0,0,0),
                     vec3(0,1,0))
rotmatrix = vmatrix.toRotMatrix3()
mvp_matrix = vmatrix*pmatrix
###################################
vtx_t = VtxV12N12B12T8C4
vbuf = vtx_t.staticBuffer(2)
vw = GBI.lock(vbuf,2)
vw.add(vtx_t(vec3(-.7,.78,0.5),vec3(),vec3(),vec2(),0xffffffff))
vw.add(vtx_t(vec3(0,0,0.5),vec3(),vec3(),vec2(),0xffffffff))
GBI.unlock(vw)
###################################
# render frame
###################################
ctx.beginFrame()
FBI.rtGroupPush(rtg)
FBI.clear(vec4(0.6,0.6,0.7,1),1.0)
ctx.debugMarker("yo")

RCFD = RenderContextFrameData(ctx)

# TODO : rework using pipeline
#sh._mtl.bindTechnique(sh._tek_frustum)
#sh.beginNoise(RCFD,0.0)
#sh.bindMvpMatrix(mvp_matrix)
#sh.bindRotMatrix(rotmatrix)
#sh.bindVolumeTex(texture)

#prim.renderEML(ctx)
#sh.end(RCFD)

#sh._mtl.bindTechnique(sh._tek_lines)
#sh.beginLines(RCFD)
#sh._mtl.bindParamMatrix4(sh._par_mvp,mtx4())
#GBI.drawLines(vw)
#sh.end(RCFD)

FontManager.beginTextBlock(ctx,"i32",vec4(0,0,.1,1),WIDTH,HEIGHT,100)
FontManager.draw(ctx,0,0,"!!! YO !!!\nThis is a textured Frustum.")
FontManager.endTextBlock(ctx)
FBI.rtGroupPop()
ctx.endFrame()
###################################
# 1. capture framebuffer -> numpy array -> PIL image
# 2. Downsample
# 3. Flip image vertically
# 4. output png
###################################
ok = FBI.captureAsFormat(rtb,capbuf,"RGBA8")
as_np = numpy.array(capbuf,dtype=numpy.uint8).reshape( AAHEIGHT, AAWIDTH, 4 )
img = Image.fromarray(as_np, 'RGBA')
img = img.resize((WIDTH,HEIGHT), Image.ANTIALIAS)
flipped = img.transpose(Image.FLIP_TOP_BOTTOM)
flipped.save("customprim.png")
run(["iv","-F","customprim.png"]) # view with openimageio viewer
