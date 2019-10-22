#!/usr/bin/env python3
import time
import serial
import serial_interface
import sys
import traceback
from itertools import count
import threading
import pandas as pd
import numpy as np
from PyQt5 import QtGui, QtCore, QtWidgets
from PyQt5.QtWidgets import QMainWindow, QAction, QPushButton, QApplication, QWidget, QLabel, QComboBox, QApplication, \
    QMessageBox, QLineEdit
from PyQt5.QtGui import QPainter, QBrush, QPen, QColor
from PyQt5.QtCore import Qt, QBasicTimer, pyqtSignal
import fileSys  # csv file
from PyQt5.Qt import QFont
from tkinter.font import BOLD

# DEFINES
WIDTH = 1440
HEIGHT = 800

BTN_W = 100
BTN_H = 50

BAUD_S = 115200
BAUD_X = 115200
BAUD_C = 115200

# Arjun board output COBs
COB_TL = 101
COB_TR = 102
COB_Psi = 201
COB_Stable = 202

# Gyro, Accel output COBs
COB_9250_AX = 111
COB_9250_AY = 112
COB_9250_AZ = 113
COB_9250_GX = 114
COB_9250_GY = 115
COB_9250_GZ = 116

COB_VOLTAGE_BASE = 0x0423
COB_VOLTAGE_MAX = 0x0429

COB_TEMP_BASE = 0x042A
COB_TEMP_MAX = 0x042D

# output COBs
COB_Steering = 121
COB_Throttle = 122

Drivers = ["SAM_RITZO", "DUKE_LOKE", "MATT_CHAI"]
Tracks = ["AutoX", "Endurance", "SkidPad"]
Fields = ['Time',
          'LeftRequest', 'RightRequest',
          'Throttle',
          'Brake',
          'SteeringAngle',
          'LeftRPM', 'RightRPM',
          'X-Accel', 'Y-Accel', 'Z-Accel',
          'X-Gyro', 'Y-Gyro', 'Z-Gyro',
          'BatteryVoltage',
          'BatteryTemp']
Strengths = ["High", "Medium", "Low"]

temps = {cell:0 for cell in range(0,28)}
volts = {cell:0 for cell in range(0,28)}

df = pd.DataFrame([[200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200]], index=['Test'],
                  columns=Fields)

GUI_ON = True

class DAQ(QMainWindow):

    def __init__(self):
        super().__init__()

        self.Driver = Drivers[0]
        self.Track = Tracks[0]
        self.Strength = Strengths[0]

        self.Ports = serial_interface.find_serial_interface_ports()
        self.file = fileSys.csvSystem(Tracks, Drivers, Fields, BAUD_C, BAUD_X, BAUD_S)

        self.Connected = True
        self.Ping = 42
        self.SteeringAngle = 120  # why is this 120?
        self.timer = QBasicTimer()
        self.testRunning = False
        self.timerSpeed = 100  # what is this for?

        try:
            self.Port = self.Ports[0]
        except:
            print("No Ports")

        self.initUI()

    def initUI(self):
        self.setWindowTitle('Formula Slug Testing')
        self.statusBar().showMessage('Status')
        self.setGeometry(0, 0, WIDTH, HEIGHT)

        if (GUI_ON):
            mainMenu = self.menuBar()
            fileMenu = mainMenu.addMenu('File')
            viewMenu = mainMenu.addMenu('View')

            # FILE MENU BUTTONS
            exitButton = QAction('Exit', self)
            exitButton.setShortcut('Ctrl+Q')
            exitButton.setStatusTip('Exit application')
            exitButton.triggered.connect(self.close)
            fileMenu.addAction(exitButton)

            deleteButton = QAction('Delete Test', self)
            deleteButton.setStatusTip('Delete Previous Test')
            deleteButton.triggered.connect(self.file.delete_file)  # TODO delete previous test file
            fileMenu.addAction(deleteButton)

            # VIEW OPTIONS
            viewStatAct = QAction('View statusbar', self, checkable=True)
            viewStatAct.setStatusTip('View statusbar')
            viewStatAct.setChecked(True)
            viewStatAct.triggered.connect(self.toggleMenu)
            viewMenu.addAction(viewStatAct)

            # TEST PARAMETERS
            comboTrack = QComboBox(self)
            for track in Tracks:
                comboTrack.addItem(track)
            comboTrack.move(WIDTH / 10, 0)
            comboTrack.adjustSize()
            comboTrack.activated[str].connect(self.changeTrack)

            comboDriver = QComboBox(self)
            for driver in Drivers:
                comboDriver.addItem(driver)
            comboDriver.move(2 * WIDTH / 10, 0)
            comboDriver.adjustSize()
            comboDriver.activated[str].connect(self.changeDriver)

            # SERIAL OPTIONS
            comboPorts = QComboBox(self)
            for port in self.Ports:
                comboPorts.addItem(port)
            comboPorts.move(4 * WIDTH / 10, 0)
            comboPorts.adjustSize()
            comboPorts.activated[str].connect(self.changePort)

            # AGGRESSIVENESS SETTING
            comboStrength = QComboBox(self)
            for strength in Strengths:
                comboStrength.addItem(strength)
            comboStrength.move(3 * WIDTH / 10 + 5, 0)
            comboStrength.adjustSize()
            comboStrength.activated[str].connect(self.changeStrength)

            # START TEST
            StartBtn = QPushButton('Start Test', self)
            StartBtn.clicked.connect(lambda: self.file.build_file(self.Driver, self.Track))
            StartBtn.clicked.connect(self.startSerial)
            StartBtn.clicked.connect(self.startTest)
            StartBtn.resize(BTN_W, BTN_H)
            StartBtn.move(WIDTH - 3 * BTN_W / 2, 5)

            mainMenu.setNativeMenuBar(False)

            self.show()

        else:  # TURN OFF THE GUI AND MAKE A BIG QUIT BUTTON
            qbtn = QPushButton('Quit', self)
            qbtn.clicked.connect(QApplication.instance().quit)
            qbtn.resize(BTN_W, BTN_H)
            qbtn.move(WIDTH / 2 - BTN_W / 2, HEIGHT / 2 - BTN_H / 2)

        self.show()

    def startRecord(self):
        self.file.csv_Add('myah')

    def closeEvent(self, event):
        # reply = QMessageBox.question(self, 'Close Message', "Are you sure to quit?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
        # if reply == QMessageBox.Yes:
        #     self.ser.close() #close the serial connection
        #     event.accept()
        # else:
        #     event.ignore()
        event.accept()  # DELETE AND UNCOMMENT TO PUT BACK ARE YOU SURE MESSAGE

    def toggleMenu(self, state):

        if state:
            self.statusBar().show()
        else:
            self.statusBar().hide()

    def changePort(self, text):
        self.Port = text

    def changeDriver(self, text):
        self.Driver = text

    def changeTrack(self, text):
        self.Track = text

    def changeStrength(self, text):
        self.Strength = text

    def printSettings(self):
        print('Driver: ' + self.Driver)
        print('Track: ' + self.Track)
        print('Port: ' + self.Port)
        print('Test Number: %s' % self.file.test_number)

    def read_from_port(self):
        while True:
            try:
                reading = self.ser.readline().decode('ascii')
                data = reading.split(' ')
                COB = int(data[0].strip(), 16)
                value = int(data[1].strip(), 16)
                print('COB:',repr(COB))
                print('Data:',repr(value))

                if COB == COB_TL:
                    df['LeftRequest'][0] = value
                    print('LeftReq')
                elif COB == COB_TR:
                    df['RightRequest'][0] = value
                    print('RightReq')
                # if COB == COB_Psi:
                #     df['LeftRPM'][0] = data[2]
                # if COB == COB_TR:
                #     df['RightRPM'][0] = data[2]
                elif COB == COB_Throttle:
                    df['Throttle'][0] = value
                elif COB == COB_Steering:
                    df['SteeringAngle'][0] = value
                elif COB == COB_9250_AX:
                    df['X-Accel'][0] = value
                    print('xaccel set', df['X-Accel'][0])
                elif COB == COB_9250_AY:
                    df['Y-Accel'][0] = value
                elif COB == COB_9250_AZ:
                    df['Z-Accel'][0] = value
                elif COB == COB_9250_GX:
                    df['X-Gyro'][0] = value
                elif COB == COB_9250_GY:
                    df['Y-Gyro'][0] = value
                elif COB == COB_9250_GZ:
                    df['Z-Gyro'][0] = value
                elif COB >= COB_VOLTAGE_BASE and COB <= COB_VOLTAGE_MAX:
                    cell_base = 4*(COB - COB_VOLTAGE_BASE)
                    for ci in range(0, 4):
                        volts[cell_base + ci] = (value >> (16*ci)) & (0xFFFF)
                elif COB >= COB_TEMP_BASE and COB <= COB_TEMP_MAX:
                    cell_base = 7*(COB - COB_TEMP_BASE)
                    for ci in range(0, 7):
                        temps[cell_base + ci] = (value >> (8*(ci+1))) & (0xFF)
                else:
                    print("THE FUCK CAN:", COB)

            except Exception as e:
                traceback.print_exc()
                print('Serial Fault')

    def startSerial(self):
        self.ser = serial.Serial(port=self.Port, baudrate=230400)
        print('Serial Port: ' + self.ser.portstr)  # check which port was really used
        self.serialthread = threading.Thread(target=self.read_from_port)
        self.serialthread.start()

    def startTest(self):
        self.timer.start(self.timerSpeed, self)

    def paintEvent(self, e):
        qp = QPainter()
        path = QtGui.QPainterPath()
        qp.begin(self)
        self.drawGUI(qp, path)
        qp.end()

    def drawGUI(self, qp, path):

        qp.setPen(QtGui.QPen(QtCore.Qt.black, 1, QtCore.Qt.SolidLine))

        # Xbee Connection
        if (self.Connected):
            qp.setBrush(QColor(100, 100, 100))
            text = ('   Ping: %s' % self.Ping)
        else:
            qp.setBrush(QColor(255, 0, 0))
            text = 'Disconnected'
        qp.drawRect(0, 55, WIDTH, 10)
        qp.drawRect(WIDTH - BTN_W * 1.5, 55, BTN_W, BTN_H)
        qp.drawText(WIDTH - BTN_W * 1.5 + 5, 80, text)

        # TOP BOX

        # Steering
        steering_w = 400
        steering_h = 400
        steering_x = WIDTH / 2 - steering_w / 2
        steering_y = 2 * HEIGHT / 5 - steering_h / 2 - 30
        steering_a = 0

        r = QtCore.QRectF(steering_x, steering_y, steering_w, steering_h)
        qp.setBrush(QColor(0, 0, 0, 0))

        qp.setPen(QtGui.QPen(QtCore.Qt.black, 3, QtCore.Qt.SolidLine))
        qp.setBrush(QColor(255, 100, 100, 255))

        # Draw the steering angle Arc
        path.moveTo(steering_x, steering_y + steering_h / 2)
        path.arcTo(r, steering_a, 180)
        path.moveTo(steering_x + steering_w / 2, steering_y + steering_h / 2)
        path.lineTo(steering_x + steering_w / 2, steering_y)
        qp.drawPath(path)

        line = QtCore.QLineF()
        line.setP1(QtCore.QPointF(steering_x + steering_w / 2, steering_y + steering_h / 2))
        line.setLength(steering_h / 2);
        line.setAngle(45);
        qp.drawLine(line);
        line.setAngle(90 + 45);
        qp.drawLine(line);

        # Draw the Steering Angle Line
        qp.setPen(QtGui.QPen(QtCore.Qt.blue, 3, QtCore.Qt.SolidLine))
        qp.setBrush(QColor(255, 255, 255, 255))
        Steering_line = QtCore.QLineF()
        Steering_line.setP1(QtCore.QPointF(steering_x + steering_w / 2, steering_y + steering_h / 2))
        Steering_line.setAngle(df['SteeringAngle'][0]-20);
        Steering_line.setLength(steering_h / 2 );
        qp.drawLine(Steering_line);

        qp.setBrush(QColor(255, 255, 255, 255))
        radius = 40
        # qp.drawEllipse(steering_x+steering_w/2-radius,steering_y+steering_h/2-radius,radius*2,radius*2)
        temp = QtCore.QRectF(steering_x + steering_w / 2 - radius, steering_y + steering_h / 2 - radius, radius * 2,
                             radius * 2)
        path.moveTo(steering_x + steering_w / 2, steering_y + steering_h / 2)
        path.arcTo(temp, steering_a, 180)

        # Draw the top bars
        qp.setPen(QtGui.QPen(QtCore.Qt.black, 3, QtCore.Qt.SolidLine))

        for i in range(2):
            self.label0 = Fields[6+i]
            qp.drawRect(1100+100*i,110,50,200)
            qp.drawText(1100+i*100, 340, self.label0)

        # # List Labels
        for i in range(2):
            self.label0 = Fields[14 + i]
            qp.drawText(200, 200 + i * 50, self.label0)

        # Draw Accel Stuff
        accel_w = 600
        accel_h = 50
        accel_x = WIDTH / 2 - accel_w / 2
        accel_y = HEIGHT / 2 - 90
        accel_a = 0

        r = QtCore.QRectF(accel_x, accel_y, accel_w, accel_h)
        qp.setPen(QtGui.QPen(QtCore.Qt.black, 3, QtCore.Qt.SolidLine))
        qp.setBrush(QColor(0, 0, 0, 0))
        qp.drawRect(r)

        # Draw Motor Stuff
        motor_w = 500
        motor_h = 375
        motor_x = WIDTH / 3.5
        motor_y = HEIGHT / 2
        motor_a = 0

        r = QtCore.QRectF(motor_x, motor_y, motor_w, motor_h)
        qp.setPen(QtGui.QPen(QtCore.Qt.black, 1, QtCore.Qt.SolidLine))
        qp.setBrush(QColor(0, 0, 0, 0))
        # qp.drawRect(r)

        qp.setPen(QtGui.QPen(QtCore.Qt.black, 1, QtCore.Qt.SolidLine))
        qp.setBrush(QColor(255, 255, 255, 255))

        # qp.drawLine(motor_x,motor_y,motor_x+motor_w,motor_y)

        self.barList = [df['X-Accel'][0], df['LeftRequest'][0], df['Throttle'][0], df['RightRequest'][0],
                        df['Y-Accel'][0], df['Z-Accel'][0]]
        print(self.barList)

        # BOTTOM BOX
        # Draw the measurement bars
        # Bottom Bar Labels
        for i in range(6):
            self.barList[i] /= (65535 / motor_h)
            self.label1 = Fields[8+i]
            qp.drawRect(motor_x + i * motor_w / 4 - motor_w / 24, motor_y + motor_h, motor_w / 12, -self.barList[i])
            qp.drawText(motor_x + (i)* motor_h / 3 - motor_h/12, motor_y + motor_h * 1.2, self.label1)

        # Draw tick marks (thick)
        for i in range(3):
            for j in range(6):
                qp.drawRect(motor_x + j * motor_w / 4 - motor_w / 12, motor_y + i * motor_h / 2 - 5, motor_w / 6, 10)

        # Draw small tick marks
        for i in range(2):
            for j in range(6):
                qp.drawRect(motor_x + j * motor_w / 4 - motor_w / 18, motor_y + i * motor_h / 2 + motor_h / 4 - 2.5,
                            motor_w / 9, 5)

        for row in range(0, 7):
            for col in range(0, 4):
                cell = row * 4 + col
                qp.drawText(30+col*75, 300+row*60, str(volts[cell])+' mV')
                qp.drawText(30+col*75, 320+row*60, str(temps[cell])+'\u00b0C')

        #qp.drawLine(WIDTH / 2, 0, WIDTH / 2, HEIGHT)
        #qp.drawLine(0, HEIGHT / 2, WIDTH, HEIGHT / 2)

        self.sleep(500)

    def sleep(self, period):
        nexttime = time.time() + period
        for i in count():
            now = time.time()
            tosleep = nexttime - now
            if tosleep > 0:
                time.sleep(tosleep)
                nexttime += period
            else:
                nexttime = now + period
            yield i, nexttime

    def timerEvent(self, event):
        if event.timerId() == self.timer.timerId():
            self.repaint()


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    ex = DAQ()
    ex.showMaximized()
    sys.exit(app.exec_())

# import time
# import serial
# import serial_interface
# import sys
# from itertools import count
# import threading
# import pandas as pd
# import numpy as np
# from PyQt5 import QtGui, QtCore, QtWidgets
# from PyQt5.QtWidgets import QMainWindow, QAction, QPushButton, QApplication, QWidget, QLabel, QComboBox, \
#     QApplication, QMessageBox, QLineEdit, QHBoxLayout, QVBoxLayout, QGridLayout
# from PyQt5.QtGui import QPainter, QBrush, QPen, QColor
# from PyQt5.QtCore import Qt, QBasicTimer, pyqtSignal
# import fileSys  # csv file
# from PyQt5.Qt import QFont
#
# # DEFINES
# WIDTH = 1440
# HEIGHT = 800
#
# BTN_W = 100
# BTN_H = 50
#
# BAUD_S = 115200
# BAUD_X = 115200
# BAUD_C = 115200
#
# # Arjun board output COBs
# COB_TL = 101
# COB_TR = 102
# COB_Psi = 201
# COB_Stable = 202
#
# # Gyro, Accel output COBs
# COB_9250_AX = 111
# COB_9250_AY = 112
# COB_9250_AZ = 113
# COB_9250_GX = 114
# COB_9250_GY = 115
# COB_9250_GZ = 116
#
# # output COBs
# COB_Steering = 121
# COB_Throttle = 122
#
# Drivers = ["SAM_RITZO", "DUKE_LOKE", "MATT_CHAI"]
# Tracks = ["AutoX", "Endurance", "SkidPad"]
# Fields = ['Time',
#           'LeftRequest', 'RightRequest',
#           'Throttle',
#           'Brake',
#           'SteeringAngle',
#           'LeftRPM', 'RightRPM',
#           'X-Accel', 'Y-Accel', 'Z-Accel',
#           'X-Gyro', 'Y-Gyro', 'Z-Gyro',
#           'BatteryVoltage',
#           'BatteryTemp']
# Strengths = ["High", "Medium", "Low"]
#
# # Update cobIDs according to CAN reference
# # Change to Box layout
# # Transmit aggressiveness serial message
# df = pd.DataFrame([[200, 200, 200, 200, 200, 200, 200, 200, 200,
#                     200, 200, 200, 200, 200, 200, 200]], index=['Test'], columns=Fields)
#
# GUI_ON = True
#
# class DAQ(QMainWindow):
#
#     def __init__(self):
#         super().__init__()
#
#         self.initUI()
#
#     def initUI(self):
#
#         self.gui = GUI(self)
#         self.setCentralWidget(self.gui)
#         self.setWindowTitle('Formula Slug Testing')
#         self.statusBar().showMessage('Ready')
#
#         if (GUI_ON):
#             mainMenu = self.menuBar()
#             fileMenu = mainMenu.addMenu('File')
#             viewMenu = mainMenu.addMenu('View')
#             mainMenu.setNativeMenuBar(False)
#
#             # FILE MENU BUTTONS
#             exitButton = QAction('Exit', self)
#             exitButton.setShortcut('Ctrl+Q')
#             exitButton.setStatusTip('Exit application')
#             exitButton.triggered.connect(self.close)
#             fileMenu.addAction(exitButton)
#
#             deleteButton = QAction('Delete Test', self)
#             deleteButton.setStatusTip('Delete Previous Test')
#             deleteButton.triggered.connect(self.file.delete_file)
#             # need to delete previous test file
#             fileMenu.addAction(deleteButton)
#
#             # VIEW OPTIONS
#             viewStatAct = QAction('View statusbar', self, checkable=True)
#             viewStatAct.setStatusTip('View statusbar')
#             viewStatAct.setChecked(True)
#             viewStatAct.triggered.connect(self.toggleMenu)
#             viewMenu.addAction(viewStatAct)
#
#             # START TEST
#             StartBtn = QPushButton('Start Test', self)
#             StartBtn.clicked.connect(lambda: self.file.build_file(self.Driver, self.Track))
#             StartBtn.clicked.connect(self.startSerial)
#             StartBtn.clicked.connect(self.startTest)
#             StartBtn.resize(BTN_W, BTN_H)
#             StartBtn.move(WIDTH - 3*BTN_W / 2, 5)
#
#         else:  # TURN OFF THE GUI AND MAKE A BIG QUIT BUTTON
#             qbtn = QPushButton('Quit', self)
#             qbtn.clicked.connect(QApplication.instance().quit)
#             qbtn.resize(BTN_W, BTN_H)
#            # qbtn.move(WIDTH / 2 - BTN_W / 2, HEIGHT / 2 - BTN_H / 2)
#
#     def toggleMenu(self, state):
#         if state:
#             self.statusBar().show()
#         else:
#             self.statusBar().hide()
#
#     def startRecord(self):
#
#         self.file.csv_Add('myah')
#
#     def closeEvent(self, event):
#         # reply = QMessageBox.question(self, 'Close Message',
#         # "Are you sure to quit?", QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
#         # if reply == QMessageBox.Yes:
#         #     self.ser.close() #close the serial connection
#         #     event.accept()
#         # else:
#         #     event.ignore()
#         event.accept()  # DELETE AND UNCOMMENT TO PUT BACK ARE YOU SURE MESSAGE
#
# class GUI(QWidget):
#
#     def __init__(self, parent):
#         super().__init__(parent)
#
#         self.Driver = Drivers[0]
#         self.Track = Tracks[0]
#         self.Strength = Strengths[0]
#
#         self.Ports = serial_interface.find_serial_interface_ports()
#         self.file = fileSys.csvSystem(Tracks, Drivers, Fields, BAUD_C, BAUD_X, BAUD_S)
#
#         self.Connected = True
#         self.Ping = 42
#         self.SteeringAngle = 120  # why is this 120?
#         self.timer = QBasicTimer()
#         self.testRunning = False
#         self.timerSpeed = 100  # what is this for?
#
#         try:
#             self.Port = self.Ports[1]
#         except:
#             print("No Ports")
#
#         self.initGUI()
#
#     def initGUI(self):
#
#         # TEST PARAMETERS
#         comboTrack = QComboBox(self)
#         for track in Tracks:
#             comboTrack.addItem(track)
#         #            comboTrack.move(WIDTH / 10, 0)
#         comboTrack.adjustSize()
#         comboTrack.activated[str].connect(self.changeTrack)
#
#         comboDriver = QComboBox(self)
#         for driver in Drivers:
#             comboDriver.addItem(driver)
#         #            comboDriver.move(2 * WIDTH / 10, 0)
#         comboDriver.adjustSize()
#         comboDriver.activated[str].connect(self.changeDriver)
#
#         # AGGRESSIVENESS SETTING
#         comboStrength = QComboBox(self)
#         for strength in Strengths:
#             comboStrength.addItem(strength)
#         #            comboStrength.move(5 * WIDTH / 10, 0)
#         comboStrength.adjustSize()
#         comboStrength.activated[str].connect(self.changeStrength)
#
#         hboxtoprow = QHBoxLayout()
#         hboxtoprow.addWidget(comboTrack)
#         hboxtoprow.addWidget(comboDriver)
#         hboxtoprow.addWidget(comboStrength)
#
#         vbox1 = QVBoxLayout()
#         vbox1.addWidget(QLabel(Fields[14]))
#         vbox1.addWidget(QLabel(Fields[15]))
#
#         hbox4 = QHBoxLayout()
#         hbox4.addWidget(QLabel(Fields[6]))
#         hbox4.addWidget(QLabel(Fields[7]))
#
#         hboxmidrow = QHBoxLayout()
#         hboxmidrow.addLayout(vbox1)
#         temporarytext = QLabel("lkjh")
#         hboxmidrow.addWidget(temporarytext)
#         hboxmidrow.addLayout(hbox4)
#
#         hboxbottrow = QHBoxLayout()
#         hboxbottrow.addWidget(QLabel(Fields[8]))
#         hboxbottrow.addWidget(QLabel(Fields[9]))
#         hboxbottrow.addWidget(QLabel(Fields[10]))
#         hboxbottrow.addWidget(QLabel(Fields[11]))
#         hboxbottrow.addWidget(QLabel(Fields[12]))
#         hboxbottrow.addWidget(QLabel(Fields[13]))
#
#         vbox = QVBoxLayout()
#         vbox.addLayout(hboxtoprow)
#         vbox.addLayout(hboxmidrow)
#         vbox.addLayout(hboxbottrow)
#
#         self.setLayout(vbox)
#
#
#     def changePort(self, text):
#         self.Port = text
#
#     def changeDriver(self, text):
#         self.Driver = text
#
#     def changeTrack(self, text):
#         self.Track = text
#
#     def changeStrength(self, text):
#         self.Strength = text
#
#     def printSettings(self):
#         print('Driver: ' + self.Driver)
#         print('Track: ' + self.Track)
#         print('Port: ' + self.Port)
#         print('Test Number: %s' % self.file.test_number)
#
#     def read_from_port(self):
#         while True:
#             try:
#                 reading = self.ser.readline().decode('ascii')
#                 data = reading.split(':')
#                 COB = int(data[1])
#                 value = int(data[2])
#                 # print('COB:',repr(COB))
#                 # print('Data:',repr(value))
#
#                 if COB == COB_TL:
#                     df['LeftRequest'][0] = value
#                     print('LeftReq')
#                 elif COB == COB_TR:
#                     df['RightRequest'][0] = value
#                     print('RightReq')
#                 # if COB == COB_Psi:
#                 #     df['LeftRPM'][0] = data[2]
#                 # if COB == COB_TR:
#                 #     df['RightRPM'][0] = data[2]
#                 elif COB == COB_Throttle:
#                     df['Throttle'][0] = value
#                 elif COB == COB_Steering:
#                     df['SteeringAngle'][0] = value
#                 elif COB == COB_9250_AX:
#                     df['X-Accel'][0] = value
#                     print('xaccel set', df['X-Accel'][0])
#                 elif COB == COB_9250_AY:
#                     df['Y-Accel'][0] = value
#                 elif COB == COB_9250_AZ:
#                     df['Z-Accel'][0] = value
#                 elif COB == COB_9250_GX:
#                     df['X-Gyro'][0] = value
#                 elif COB == COB_9250_GY:
#                     df['Y-Gyro'][0] = value
#                 elif COB == COB_9250_GZ:
#                     df['Z-Gyro'][0] = value
#                 else:
#                     print("THE FUCK CAN:", COB)
#
#             except:
#                 print('Serial Fault')
#
#     def startSerial(self):
#         self.ser = serial.Serial(port=self.Port, baudrate=BAUD_S)
#         print('Serial Port: ' + self.ser.portstr)  # check which port was really used
#         self.serialthread = threading.Thread(target=self.read_from_port)
#         self.serialthread.start()
#
#     def startTest(self):
#         self.timer.start(self.timerSpeed, self)
#
#     def paintEvent(self, e):
#         qp = QPainter()
#         path = QtGui.QPainterPath()
#         qp.begin(self)
#         self.drawGUI(qp, path)
#         qp.end()
#
#     def drawGUI(self, qp, path):
#
#         qp.setPen(QtGui.QPen(QtCore.Qt.black, 1, QtCore.Qt.SolidLine))
#
#         # Xbee Connection
#         if (self.Connected):
#             qp.setBrush(QColor(0, 255, 0))
#             text = ('      Ping: %s' % self.Ping)
#         else:
#             qp.setBrush(QColor(255, 0, 0))
#             text = 'Disconnected'
#         qp.drawRect(0, 55, WIDTH, 10)
#         qp.drawRect(WIDTH - BTN_W * 1.5, 55, BTN_W, BTN_H)
#         qp.drawText(WIDTH - BTN_W * 1.5 + 5, 80, text)
#
#         # TOP BOX
#         # Steering
#         steering_w = 400
#         steering_h = 400
#         steering_x = WIDTH / 2 - steering_w / 2
#         steering_y = 2 * HEIGHT / 5 - steering_h / 2 - 30
#         steering_a = 0
#
#         r = QtCore.QRectF(steering_x, steering_y, steering_w, steering_h)
#         qp.setBrush(QColor(0, 0, 0, 0))
#         # qp.drawRect(r)
#
#         qp.setPen(QtGui.QPen(QtCore.Qt.black, 3, QtCore.Qt.SolidLine))
#         qp.setBrush(QColor(255, 100, 100, 255))
#
#         # Draw the steering angle Arc
#         path.moveTo(steering_x, steering_y + steering_h / 2)
#         path.arcTo(r, steering_a, 180)
#         path.moveTo(steering_x + steering_w / 2, steering_y + steering_h / 2)
#         path.lineTo(steering_x + steering_w / 2, steering_y)
#         qp.drawPath(path)
#
#         line = QtCore.QLineF()
#         line.setP1(QtCore.QPointF(steering_x + steering_w / 2, steering_y + steering_h / 2))
#         line.setLength(steering_h / 2)
#         line.setAngle(45)
#         qp.drawLine(line)
#         line.setAngle(90 + 45)
#         qp.drawLine(line)
#
#         # Draw the Steering Angle Line
#         qp.setPen(QtGui.QPen(QtCore.Qt.blue, 3, QtCore.Qt.SolidLine))
#         qp.setBrush(QColor(255, 255, 255, 255))
#         Steering_line = QtCore.QLineF()
#         Steering_line.setP1(QtCore.QPointF(steering_x + steering_w / 2, steering_y + steering_h / 2))
#         Steering_line.setAngle(df['SteeringAngle'][0])
#         Steering_line.setLength(steering_h / 2 + 5)
#         qp.drawLine(Steering_line)
#
#         qp.setBrush(QColor(255, 255, 255, 255))
#         radius = 40
#         # qp.drawEllipse(steering_x+steering_w/2-radius,steering_y+steering_h/2-radius,radius*2,radius*2)
#         temp = QtCore.QRectF(steering_x + steering_w / 2 - radius, steering_y + steering_h / 2 - radius, radius * 2,
#                              radius * 2)
#         path.moveTo(steering_x + steering_w / 2, steering_y + steering_h / 2)
#         path.arcTo(temp, steering_a, 180)
#
#         # Draw Accel Stuff
#         accel_w = 600
#         accel_h = 50
#         accel_x = WIDTH / 2 - accel_w / 2
#         accel_y = HEIGHT / 2 - 90
#         accel_a = 0
#
#         r = QtCore.QRectF(accel_x, accel_y, accel_w, accel_h)
#         qp.setPen(QtGui.QPen(QtCore.Qt.black, 3, QtCore.Qt.SolidLine))
#         qp.setBrush(QColor(0, 0, 0, 0))
#         qp.drawRect(r)
#
#         # Draw Motor Stuff
#         motor_w = 500
#         motor_h = 375
#         motor_x = WIDTH / 2 - motor_w / 2
#         motor_y = HEIGHT / 2 - 20
#         motor_a = 0
#
#         r = QtCore.QRectF(motor_x, motor_y, motor_w, motor_h)
#         qp.setPen(QtGui.QPen(QtCore.Qt.black, 1, QtCore.Qt.SolidLine))
#         qp.setBrush(QColor(0, 0, 0, 0))
#         # qp.drawRect(r)
#
#         qp.setPen(QtGui.QPen(QtCore.Qt.black, 1, QtCore.Qt.SolidLine))
#         qp.setBrush(QColor(255, 255, 255, 255))
#
#         # qp.drawLine(motor_x,motor_y,motor_x+motor_w,motor_y)
#
#         self.barList = [df['X-Accel'][0], df['LeftRequest'][0], df['Throttle'][0], df['RightRequest'][0],
#                         df['Y-Accel'][0], df['Z-Accel'][0]]
#         # print(self.barList)
#
#         # BOTTOM BOX
#         # Draw the measurement bars
#         for i in range(6):
#             self.barList[i] /= (65535 / motor_h)
#             qp.drawRect(motor_x + i * motor_w / 4 - motor_w / 24, motor_y + motor_h, motor_w / 12, -self.barList[i])
#             qp.drawLine(motor_x, motor_y + (i) * motor_h / 4, motor_x + motor_w, motor_y + (i) * motor_h / 4)
#
#         # Draw tick marks (thick)
#         for i in range(3):
#             for j in range(6):
#                 qp.drawRect(motor_x + j * motor_w / 4 - motor_w / 12, motor_y + i * motor_h / 2 - 5, motor_w / 6, 10)
#
#         # Draw small tick marks
#         for i in range(2):
#             for j in range(6):
#                 qp.drawRect(motor_x + j * motor_w / 4 - motor_w / 18, motor_y + i * motor_h / 2 + motor_h / 4 - 2.5,
#                             motor_w / 9, 5)
#
#         qp.drawLine(WIDTH / 2, 0, WIDTH / 2, HEIGHT)
#         qp.drawLine(0, HEIGHT / 2, WIDTH, HEIGHT / 2)
#
#         self.sleep(3000)
#
#     def sleep(self, period):
#         nexttime = time.time() + period
#         for i in count():
#             now = time.time()
#             tosleep = nexttime - now
#             if tosleep > 0:
#                 time.sleep(tosleep)
#                 nexttime += period
#             else:
#                 nexttime = now + period
#             yield i, nexttime
#
#     def timerEvent(self, event):
#         if event.timerId() == self.timer.timerId():
#             self.repaint()
#
# if __name__ == '__main__':
#     app = QApplication(sys.argv)
#     gui = DAQ()
#     gui.showMaximized()
#     sys.exit(app.exec_())
