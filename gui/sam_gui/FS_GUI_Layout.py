from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
import pyqtgraph.opengl as gl
import numpy as np
import math

import FS_GUI_BMS

class gui:
	def __init__(self):
		# Main App
		self.app = QApplication([])
		self.window = QWidget()
		self.window.setGeometry(QRect(600,200,800,400))

		# Main App Layout
		self.main_layout = QGridLayout()
		self.window.setLayout(self.main_layout)

		# Main App Layout Tabs
		self.tabs = QTabWidget()
		self.main_layout.addWidget(self.tabs,1,1)

		# Tab 1
		self.BMS = FS_GUI_BMS.FS_GUI_BMS()
		self.tab1 = self.BMS.widget
		self.tabs.addTab(self.tab1,'BMS')

		self.tab2 = None

		self.window.show()

	def exec(self):
		self.app.exec_()