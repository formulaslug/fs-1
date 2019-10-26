from PyQt5.QtWidgets import *
from PyQt5.QtCore import *

class FS_GUI_BMS:
	def __init__(self):
		self.widget = QWidget()
		self.batt_grid_layout = QGridLayout()
		self.batt_grid = [[QLabel("{} {}".format(x,y)) for x in range(4)] for y in range(7)]
		for x in range(4):
			for y in range(7):
				self.batt_grid_layout.addWidget(self.batt_grid[y][x],x,y)
		self.widget.setLayout(self.batt_grid_layout)