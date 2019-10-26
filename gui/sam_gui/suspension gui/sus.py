import sys
import math
import sus_math
import sus_gui
import pyqtgraph.opengl as gl

gui = sus_gui.gui()
gui.make_gui()

gui.member_UF = gl.GLLinePlotItem(antialias = 1, color = (1,1,0,1), width = 2)
gui.view3d.addItem(gui.member_UF)
gui.member_LF = gl.GLLinePlotItem(antialias = 1, color = (1,0,1,1), width = 2)
gui.view3d.addItem(gui.member_LF)

gui.member_UR = gl.GLLinePlotItem(antialias = 1, color = (0,1,1,1), width = 2)
gui.view3d.addItem(gui.member_UR)
gui.member_LR = gl.GLLinePlotItem(antialias = 1, color = (0,0,1,1), width = 2)
gui.view3d.addItem(gui.member_LR)

gui.member_U = gl.GLLinePlotItem(antialias = 1, color = (1,0,0,1), width = 2)
gui.view3d.addItem(gui.member_U)

gui.wheel = gl.MeshData.cylinder(10, 10, radius = [18, 18])

gui.defaults()
gui.DSB_update()
gui.exec()