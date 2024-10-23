import sys

from PyQt5.QtWidgets import QApplication, QMessageBox
from PyQt5.QtCore import pyqtSignal

from pyFormGen.units import convert
from pyFileIO import fileIO

from lib.sensorProfileManager import SensorProfileManager
from lib.preferencesManager import PreferencesManager
from lib.logger import logger
from lib.fileTypes import FILE_TYPES
from lib.migrations import *

from ui.mainWindow import MainWindow

class App(QApplication):

    NAME = 'RMTS'
    VERSION = (0, 3, 2)

    newConverter = pyqtSignal(object)
    newFiringConfig = pyqtSignal(object)

    def __init__(self, args):
        super().__init__(args)
        self.setupFileIO()
        self.sensorProfileManager = SensorProfileManager()
        self.sensorProfileManager.loadProfiles()
        self.preferencesManager = PreferencesManager()
        self.preferencesManager.loadPreferences()

        logger.log('Application version: {}.{}.{}'.format(*self.VERSION))

        self.window = MainWindow(self)
        logger.log('Showing window')
        self.window.show()

    def outputMessage(self, content, title='RMTS'):
        msg = QMessageBox()
        msg.setText(content)
        msg.setWindowTitle(title)
        msg.exec_()

    def outputException(self, exception, text, title='RMTS - Error'):
        msg = QMessageBox()
        msg.setText(text)
        msg.setInformativeText(str(exception))
        msg.setWindowTitle(title)
        msg.exec_()

    def convertToUserUnits(self, value, units):
        return self.preferencesManager.preferences.convert(value, units)

    def convertFromUserUnits(self, value, baseUnit):
        inUnit = self.preferencesManager.preferences.getUnit(baseUnit)
        return convert(value, inUnit, baseUnit)

    def convertAllToUserUnits(self, values, units):
        return self.preferencesManager.preferences.convertAll(values, units)

    def convertToUserAndFormat(self, value, units, places):
        return self.preferencesManager.preferences.convFormat(value, units, places)

    def getUserUnit(self, unit):
        return self.preferencesManager.preferences.getUnit(unit)

    def getPreferences(self):
        return self.preferencesManager.preferences

    def newResult(self, motorInfo, fileName = None):
        self.window.ui.pageResults.showResults(motorInfo, fileName)

    def configureLiveResults(self, size):
        self.window.ui.pageResults.setupLive(size)

    # No packet argument because this is just for resetting data age
    def newResultsPacket(self):
        self.window.ui.pageResults.newResultsPacket()

    def newCalibration(self, calibration):
        self.window.ui.pageCalibration.newCalibration(calibration)

    def setupFileIO(self):
        fileIO.setAppInfo(self.NAME, self.VERSION)
        fileIO.registerFileType(FILE_TYPES.PREFERENCES)
        fileIO.registerFileType(FILE_TYPES.TRANSDUCERS)
        fileIO.registerFileType(FILE_TYPES.FIRING)

        fileIO.registerMigration(FILE_TYPES.PREFERENCES, (0, 1, 0), (0, 2, 0), lambda data: data)
        fileIO.registerMigration(FILE_TYPES.TRANSDUCERS, (0, 1, 0), (0, 2, 0), lambda data: data)
        fileIO.registerMigration(FILE_TYPES.FIRING, (0, 1, 0), (0, 2, 0), migrateFiring_0_1_0_to_0_2_0)

        fileIO.registerMigration(FILE_TYPES.PREFERENCES, (0, 2, 0), (0, 3, 0), lambda data: data)
        fileIO.registerMigration(FILE_TYPES.TRANSDUCERS, (0, 2, 0), (0, 3, 0), lambda data: data)
        fileIO.registerMigration(FILE_TYPES.FIRING, (0, 2, 0), (0, 3, 0), lambda data: data)

        fileIO.registerMigration(FILE_TYPES.PREFERENCES, (0, 3, 0), (0, 3, 1), lambda data: data)
        fileIO.registerMigration(FILE_TYPES.TRANSDUCERS, (0, 3, 0), (0, 3, 1), lambda data: data)
        fileIO.registerMigration(FILE_TYPES.FIRING, (0, 3, 0), (0, 3, 1), lambda data: data)

        fileIO.registerMigration(FILE_TYPES.PREFERENCES, (0, 3, 1), (0, 3, 2), lambda data: data)
        fileIO.registerMigration(FILE_TYPES.TRANSDUCERS, (0, 3, 1), (0, 3, 2), lambda data: data)
        fileIO.registerMigration(FILE_TYPES.FIRING, (0, 3, 1), (0, 3, 2), lambda data: data)
