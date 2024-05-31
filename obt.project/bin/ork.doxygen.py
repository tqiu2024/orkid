#!/usr/bin/env python3
##########################################################################
# run doxygen on unioned filetree
##########################################################################

import os, string, pathlib

from pathlib import Path
from obt import template, command, path

doxy_output_dir = path.builds()/"orkid"/"doxygen"
src_doxyfile = path.orkid()/"ork.dox"/"Doxyfile"
dst_doxyfile = path.builds()/"orkid"/"Doxyfile"

command.run(["ork.createunionedsource.py"]) # first we definitely need unioned src (for ease...)

###########################################################
# fix up icon path
###########################################################

src_icon_path = path.Path(os.environ["ORKID_WORKSPACE_DIR"])/"ork.data"/"dox"/"doxylogo.png"

template.template_file(src_doxyfile,
                       {"ICONPATH":src_icon_path},
                       dst_doxyfile)

###########################################################

os.chdir(path.stage()/"orkid")
command.run(["rm","-rf",doxy_output_dir])
command.run(["doxygen",dst_doxyfile])
