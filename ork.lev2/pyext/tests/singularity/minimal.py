#!/usr/bin/env python3

################################################################################
# lev2 sample which renders a UI with four views to the same scenegraph to a window
# Copyright 1996-2023, Michael T. Mayers.
# Distributed under the MIT License
# see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
################################################################################

import sys, math, random, numpy, obt.path, time, bisect
import plotly.graph_objects as go
from collections import defaultdict
import re
from orkengine.core import *
from orkengine.lev2 import *
################################################################################
sys.path.append((thisdir()/"..").normalized.as_string) # add parent dir to path
from _boilerplate import *
################################################################################

def find_index(sorted_list, value):
    index = bisect.bisect_left(sorted_list, value)
    if index != len(sorted_list) and sorted_list[index] == value:
        return index
    return -1  # Return -1 or some other value to indicate "not found"

class SingulApp(object):

  def __init__(self):
    super().__init__()
    self.ezapp = OrkEzApp.create(self,left=420, top=100, height=720,width=480)
    self.ezapp.setRefreshPolicy(RefreshFastest, 0)
    self.ezapp.topWidget.enableUiDraw()
    lg_group = self.ezapp.topLayoutGroup
    root_layout = lg_group.layout
    self.time = 0.0
  
  ##############################################

  def onGpuInit(self,ctx):
    self.context = ctx
    self.audiodevice = singularity.device.instance()
    self.synth = singularity.synth.instance()
    self.mainbus = self.synth.outputBus("main")
    for i in range(6):
      self.synth.nextEffect()
    self.syn_data_base = singularity.baseDataPath()/"casioCZ"
    self.synth.masterGain = singularity.decibelsToLinear(-12.0)
    self.czdata = singularity.CzSynthData()
    self.czdata.loadBank("bankA", self.syn_data_base/"factoryA.bnk")
    self.czdata.loadBank("bankB", self.syn_data_base/"factoryB.bnk")
    #self.czdata.loadBank("bank0", self.syn_data_base/"cz1_1.bnk")
    #self.czdata.loadBank("bank1", self.syn_data_base/"cz1_2.bnk")
    #self.czdata.loadBank("bank2", self.syn_data_base/"cz1_3.bnk")
    #self.czdata.loadBank("bank3", self.syn_data_base/"cz1_4.bnk")
    self.czbank = self.czdata.bankData
    self.czprogs = self.czbank.programsByName
    self.sorted_progs = sorted(self.czprogs.keys())
    self.octave = 3
    self.charts = {}

    print("czprogs<%s>" % self.czprogs)
    self.mylist = [
      "Bells and Chimes",
      "ORCHESTRA",
      "Sizzle Cymbal"
    ]
    # find index of "Bells and Chimes" in sorted_progs
    self.prog_index = find_index(self.sorted_progs, "Sizzle Cymbal")
    print("prog_index<%d>" % self.prog_index)

    self.base_notes = {
        ord("A"): 0,
        ord("W"): 1,
        ord("S"): 2,
        ord("E"): 3,
        ord("D"): 4,
        ord("F"): 5,
        ord("T"): 6,
        ord("G"): 7,
        ord("Y"): 8,
        ord("H"): 9,
        ord("U"): 10,
        ord("J"): 11,
        ord("K"): 12,
        ord("O"): 13,
        ord("L"): 14,
        ord("P"): 15,
        ord(";"): 16,
        ord("'"): 17,
        ord("]"): 18,
    }
    self.voices = dict()

  def onUpdate(self,updinfo):
    self.time = updinfo.absolutetime

  def onGpuUpdate(self,ctx):
    pass

  def onUiEvent(self,uievent):
    if uievent.code == tokens.KEY_DOWN.hashed:
      KC = uievent.keycode
      if KC in self.base_notes:
       if KC not in self.voices:
         index_fixed = self.prog_index % len(self.sorted_progs)
         prgname = self.sorted_progs[index_fixed]
         self.prog = self.czbank.programByName(prgname)
         note = self.base_notes[KC] + (self.octave*12)
         mods = singularity.KeyOnModifiers()
         timebase = self.time
         modrate = math.sin(self.time)*5
         def modulatePan():
            return math.sin((self.time-timebase)*modrate)*2
         def sub(name,value):
           channel = "%g.%s" % (timebase,name)
           if channel not in self.charts:
             self.charts[channel] = dict()
           self.charts[channel][self.time-timebase] = value.x
         mods.generators = {
           "LCZX0.STEREOPAN2": modulatePan
         }
         mods.subscribers = {
           "LCZX0.DCAENV0": sub,
           "LCZX0.DCAENV1": sub
         }
         voice = self.synth.keyOn(note,127,self.prog,mods)
         self.voices[KC] = voice
      else:
        if KC == ord(","): # prev program
          self.prog_index -= 1
          if self.prog_index < 0:
            self.prog_index = len(self.sorted_progs)-1
          prgname = self.sorted_progs[self.prog_index]
          print("prgname<%s>" % prgname)
        elif KC == ord("."): # next program
          self.prog_index += 1
          if self.prog_index >= len(self.sorted_progs):
            self.prog_index = 0
          prgname = self.sorted_progs[self.prog_index]
          print("prgname<%s>" % prgname)
        elif KC == ord(" "): # next effect
          self.synth.nextEffect()
        elif KC == ord("!"): # panic
          for voice in self.voices:
            self.synth.keyOff(voice)
            self.voices.clear()
        elif KC == ord("Z"): # 
          self.octave -= 1
          if self.octave < 0:
            self.octave = 0
        elif KC == ord("X"): # 
          self.octave += 1
          if self.octave > 8:
            self.octave = 8
        elif KC == ord("N"): # new chart 
          self.charts = dict()
        elif KC == ord("M"): # show chart
          fig = go.Figure()
          for name, the_dict in self.charts.items():
            sorted_data = sorted(the_dict.items())
            times, values = zip(*sorted_data)
            fig.add_trace(go.Scatter(x=times, y=values, mode='lines', name=name))
          fig.show()
          self.charts = dict()
  
    elif uievent.code == tokens.KEY_UP.hashed:
      KC = uievent.keycode
      if KC in self.voices:
        voice = self.voices[KC]
        self.synth.keyOff(voice)
        del self.voices[KC]
      pass


###############################################################################

app = SingulApp()
app.ezapp.mainThreadLoop()
