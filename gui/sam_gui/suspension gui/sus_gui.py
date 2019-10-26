from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
import pyqtgraph.opengl as gl
import sus_math
import numpy as np
import math

yellow = (1,1,0)

class gui:
	def DSB_update(self):
		# Collect values from QDoubleSpinBox
		self.point_UF = sus_math.point(self.DSB_UFx.value(), self.DSB_UFz.value(), self.DSB_UFy.value())
		self.point_LF = sus_math.point(self.DSB_LFx.value(), self.DSB_LFz.value(), self.DSB_LFy.value())
		self.point_UR = sus_math.point(self.DSB_URx.value(), self.DSB_URz.value(), self.DSB_URy.value())
		self.point_LR = sus_math.point(self.DSB_LRx.value(), self.DSB_LRz.value(), self.DSB_LRy.value())
		self.point_UU = sus_math.point(self.DSB_UUx.value(), self.DSB_UUz.value(), self.DSB_UUy.value())
		self.point_LU = sus_math.point(self.DSB_LUx.value(), self.DSB_LUz.value(), self.DSB_LUy.value())

		self.kp_static = math.degrees(math.atan2(self.point_UU.x-self.point_LU.x,self.point_UU.z-self.point_LU.z))
		# Update GLLinePlotItem positions
		self.member_UF.setData(pos = np.array([self.point_UF.array(), self.point_UU.array()]))
		self.member_LF.setData(pos = np.array([self.point_LF.array(), self.point_LU.array()]))
		self.member_UR.setData(pos = np.array([self.point_UR.array(), self.point_UU.array()]))
		self.member_LR.setData(pos = np.array([self.point_LR.array(), self.point_LU.array()]))
		self.member_U.setData(pos = np.array([self.point_UU.array(), self.point_LU.array()]))
		# Update lengths
		self.length_UF = abs(sus_math.length(self.point_UF, self.point_UU))
		self.length_LF = abs(sus_math.length(self.point_LF, self.point_LU))
		self.length_UR = abs(sus_math.length(self.point_UR, self.point_UU))
		self.length_LR = abs(sus_math.length(self.point_LR, self.point_LU))
		self.length_U = abs(sus_math.length(self.point_UU, self.point_LU))
		# Create new points
		self.new_LU = sus_math.point(None, None, None)
		self.new_UU = sus_math.point(None, None, None)
		# Reset slider
		self.slider.setValue(0)

		# Set values
		

	def slider_update(self):
		self.new_LU.z = self.point_LU.z + (self.slider.value()/1000)
		self.new_LU.x, self.new_LU.y = sus_math.triang_2(self.point_LF, self.point_LR, self.length_LF, self.length_LR, self.new_LU.z, self.point_LU)
		self.new_UU.x, self.new_UU.y, self.new_UU.z = sus_math.triang_3(self.point_UF, self.point_UR, self.new_LU, self.length_UF, self.length_UR, self.length_U, self.point_UU)
		self.value_slider.setText('{: 04.2f}'.format(self.slider.value()/1000))

		self.out_KP.setText('{: 04.2f}'.format(sus_math.kingpin(self.new_UU, self.new_LU)))
		self.out_CAST.setText('{: 04.2f}'.format(sus_math.caster(self.new_UU, self.new_LU)))
		self.out_CAMB.setText('{: 04.2f}'.format(self.DSB_camb_static.value()+sus_math.kingpin(self.new_UU, self.new_LU)-self.kp_static))

		self.member_UF.setData(pos = np.array([self.point_UF.array(), self.new_UU.array()]))
		self.member_LF.setData(pos = np.array([self.point_LF.array(), self.new_LU.array()]))
		self.member_UR.setData(pos = np.array([self.point_UR.array(), self.new_UU.array()]))
		self.member_LR.setData(pos = np.array([self.point_LR.array(), self.new_LU.array()]))
		self.member_U.setData(pos = np.array([self.new_UU.array(), self.new_LU.array()]))

		self.first = 0

	def make_gui(self):
		self.app = QApplication([])
		self.window = QWidget()
		self.window.setGeometry(QRect(1000,600,800,400))
		self.main_layout = QGridLayout()

		# Coordinate input
		self.input_layout = QGridLayout()

		# Upper Front Sus Point
		self.GB_UF = QGroupBox("Upper Front Sus Point")
		self.GL_UF = QGridLayout()

		self.Label_UFx = QLabel("X")
		self.GL_UF.addWidget(self.Label_UFx, 0, 0)
		self.DSB_UFx = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UF.addWidget(self.DSB_UFx, 0, 1)
		self.DSB_UFx.valueChanged.connect(self.DSB_update)

		self.Label_UFy = QLabel("Y")
		self.GL_UF.addWidget(self.Label_UFy, 1, 0)
		self.DSB_UFy = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UF.addWidget(self.DSB_UFy, 1, 1)
		self.DSB_UFy.valueChanged.connect(self.DSB_update)

		self.Label_UFz = QLabel("Z")
		self.GL_UF.addWidget(self.Label_UFz, 2, 0)
		self.DSB_UFz = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UF.addWidget(self.DSB_UFz, 2, 1)
		self.DSB_UFz.valueChanged.connect(self.DSB_update)

		self.GB_UF.setLayout(self.GL_UF)
		self.input_layout.addWidget(self.GB_UF,0,0)

		# Lower Front Sus Point
		self.GB_LF = QGroupBox("Lower Front Sus Point")
		self.GL_LF = QGridLayout()

		self.Label_LFx = QLabel("X")
		self.GL_LF.addWidget(self.Label_LFx, 0, 0)
		self.DSB_LFx = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LF.addWidget(self.DSB_LFx, 0, 1)
		self.DSB_LFx.valueChanged.connect(self.DSB_update)

		self.Label_LFy = QLabel("Y")
		self.GL_LF.addWidget(self.Label_LFy, 1, 0)
		self.DSB_LFy = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LF.addWidget(self.DSB_LFy, 1, 1)
		self.DSB_LFy.valueChanged.connect(self.DSB_update)

		self.Label_LFz = QLabel("Z")
		self.GL_LF.addWidget(self.Label_LFz, 2, 0)
		self.DSB_LFz = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LF.addWidget(self.DSB_LFz, 2, 1)

		self.GB_LF.setLayout(self.GL_LF)
		self.input_layout.addWidget(self.GB_LF,1,0)
		self.DSB_LFz.valueChanged.connect(self.DSB_update)

		# Upper Rear Sus Point
		self.GB_UR = QGroupBox("Upper Rear Sus Point")
		self.GL_UR = QGridLayout()

		self.Label_URx = QLabel("X")
		self.GL_UR.addWidget(self.Label_URx, 0, 0)
		self.DSB_URx = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UR.addWidget(self.DSB_URx, 0, 1)
		self.DSB_URx.valueChanged.connect(self.DSB_update)

		self.Label_URy = QLabel("Y")
		self.GL_UR.addWidget(self.Label_URy, 1, 0)
		self.DSB_URy = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UR.addWidget(self.DSB_URy, 1, 1)
		self.DSB_URy.valueChanged.connect(self.DSB_update)

		self.Label_URz = QLabel("Z")
		self.GL_UR.addWidget(self.Label_URz, 2, 0)
		self.DSB_URz = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UR.addWidget(self.DSB_URz, 2, 1)
		self.DSB_URz.valueChanged.connect(self.DSB_update)

		self.GB_UR.setLayout(self.GL_UR)
		self.input_layout.addWidget(self.GB_UR,0,1)

		# Lower Rear Sus Point
		self.GB_LR = QGroupBox("Lower Rear Sus Point")
		self.GL_LR = QGridLayout()

		self.Label_LRx = QLabel("X")
		self.GL_LR.addWidget(self.Label_LRx, 0, 0)
		self.DSB_LRx = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LR.addWidget(self.DSB_LRx, 0, 1)
		self.DSB_LRx.valueChanged.connect(self.DSB_update)

		self.Label_LRy = QLabel("Y")
		self.GL_LR.addWidget(self.Label_LRy, 1, 0)
		self.DSB_LRy = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LR.addWidget(self.DSB_LRy, 1, 1)
		self.DSB_LRy.valueChanged.connect(self.DSB_update)

		self.Label_LRz = QLabel("Z")
		self.GL_LR.addWidget(self.Label_LRz, 2, 0)
		self.DSB_LRz = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LR.addWidget(self.DSB_LRz, 2, 1)
		self.DSB_LRz.valueChanged.connect(self.DSB_update)

		self.GB_LR.setLayout(self.GL_LR)
		self.input_layout.addWidget(self.GB_LR,1,1)

		# Upper Upright Point
		self.GB_UU = QGroupBox("Upper Upright Point")
		self.GL_UU = QGridLayout()

		self.Label_UUx = QLabel("X")
		self.GL_UU.addWidget(self.Label_UUx, 0, 0)
		self.DSB_UUx = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UU.addWidget(self.DSB_UUx, 0, 1)
		self.DSB_UUx.valueChanged.connect(self.DSB_update)

		self.Label_UUy = QLabel("Y")
		self.GL_UU.addWidget(self.Label_UUy, 1, 0)
		self.DSB_UUy = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UU.addWidget(self.DSB_UUy, 1, 1)
		self.DSB_UUy.valueChanged.connect(self.DSB_update)

		self.Label_UUz = QLabel("Z")
		self.GL_UU.addWidget(self.Label_UUz, 2, 0)
		self.DSB_UUz = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_UU.addWidget(self.DSB_UUz, 2, 1)
		self.DSB_UUz.valueChanged.connect(self.DSB_update)

		self.GB_UU.setLayout(self.GL_UU)
		self.input_layout.addWidget(self.GB_UU,0,2)

		# Lower Upright Point
		self.GB_LU = QGroupBox("Lower Upright Point")
		self.GL_LU = QGridLayout()

		self.Label_LUx = QLabel("X")
		self.GL_LU.addWidget(self.Label_LUx, 0, 0)
		self.DSB_LUx = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LU.addWidget(self.DSB_LUx, 0, 1)
		self.DSB_LUx.valueChanged.connect(self.DSB_update)

		self.Label_LUy = QLabel("Y")
		self.GL_LU.addWidget(self.Label_LUy, 1, 0)
		self.DSB_LUy = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LU.addWidget(self.DSB_LUy, 1, 1)
		self.DSB_LUy.valueChanged.connect(self.DSB_update)

		self.Label_LUz = QLabel("Z")
		self.GL_LU.addWidget(self.Label_LUz, 2, 0)
		self.DSB_LUz = QDoubleSpinBox(decimals = 3, minimum = -1000, maximum = 1000)
		self.GL_LU.addWidget(self.DSB_LUz, 2, 1)
		self.DSB_LUz.valueChanged.connect(self.DSB_update)

		self.GB_LU.setLayout(self.GL_LU)
		self.input_layout.addWidget(self.GB_LU,1,2)

		self.GB_params = QGroupBox("Other Parameters")
		self.GL_params = QGridLayout()

		self.label_camb_static = QLabel("Static Camber")
		self.GL_params.addWidget(self.label_camb_static,0,0)
		self.DSB_camb_static = QDoubleSpinBox(decimals = 3, minimum = -90, maximum = 90)
		self.GL_params.addWidget(self.DSB_camb_static,0,1)
		self.DSB_camb_static.valueChanged.connect(self.DSB_update)
		self.GB_params.setLayout(self.GL_params)

		self.input_layout.addWidget(self.GB_params,2,1)


		self.GB_OUT = QGroupBox("Outputs")
		self.GL_OUT = QGridLayout()

		self.label_KP = QLabel("Kingpin Inclination")
		self.GL_OUT.addWidget(self.label_KP,0,0)
		self.out_KP = QLabel("0.00")
		self.GL_OUT.addWidget(self.out_KP,0,1)

		self.label_CAST = QLabel("Caster Angle")
		self.GL_OUT.addWidget(self.label_CAST,1,0)
		self.out_CAST = QLabel("0.00")
		self.GL_OUT.addWidget(self.out_CAST,1,1)

		self.label_CAMB = QLabel("Camber Angle")
		self.GL_OUT.addWidget(self.label_CAMB,2,0)
		self.out_CAMB = QLabel("0.00")
		self.GL_OUT.addWidget(self.out_CAMB,2,1)

		self.GB_OUT.setLayout(self.GL_OUT)

		self.input_layout.addWidget(self.GB_OUT,2,2)

		self.input_layout.setRowStretch(3,1)

		# Rest of the stuff
		self.view3d = gl.GLViewWidget()

		# Drawing members
		self.line_UF = gl.GLLinePlotItem(color=yellow, pos=[self.DSB_UFx])

		self.GL_slider = QGridLayout()
		self.slider = QSlider(Qt.Vertical)
		self.slider.setMinimum(-5000)
		self.slider.setMaximum(5000)
		self.slider.setValue(0)
		self.slider.valueChanged.connect(self.slider_update)
		self.GL_slider.addWidget(self.slider,0,0)
		self.label_slider = QLabel("Sus\nPos")
		self.GL_slider.addWidget(self.label_slider,1,0)
		self.value_slider = QLabel("00.00")
		self.GL_slider.addWidget(self.value_slider,2,0)

		self.main_layout.addLayout(self.input_layout,0,0)
		self.main_layout.setColumnStretch(1,1)
		self.main_layout.addWidget(self.view3d,0,1)
		self.main_layout.addLayout(self.GL_slider,0,2)
		self.window.setLayout(self.main_layout)

		self.window.show()

	def defaults(self):
		self.DSB_UFx.setValue(10.49)
		self.DSB_UFy.setValue(13.38)
		self.DSB_UFz.setValue(10.30)

		self.DSB_LFx.setValue( 7.99)
		self.DSB_LFy.setValue( 5.94)
		self.DSB_LFz.setValue(10.48)

		self.DSB_URx.setValue(10.49)
		self.DSB_URy.setValue(13.38)
		self.DSB_URz.setValue( 0.75)

		self.DSB_LRx.setValue( 7.99)
		self.DSB_LRy.setValue( 5.94)
		self.DSB_LRz.setValue( 0.70)

		self.DSB_UUx.setValue(20.64)
		self.DSB_UUy.setValue(13.22)
		self.DSB_UUz.setValue( 5.00)

		self.DSB_LUx.setValue(21.58)
		self.DSB_LUy.setValue( 6.25)
		self.DSB_LUz.setValue( 5.73)

	def exec(self):
		self.app.exec_()