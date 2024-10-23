from enum import Enum
from math import ceil
from threading import Thread
from time import sleep

from PyQt5.QtCore import QObject, pyqtSignal

from pyFileIO import fileIO

from .converter import Converter
from .radio import RadioManager, SetupPacket, FirePacket, ResultPacket, ErrorPacket, StopPacket, VersionPacket, FiringPacket
from .motor import processRawData
from .firmwareVersions import checkVersionPacket
from .logger import logger

PACKET_STRIDE = 10

class VERSION_CHECK_STATE(Enum):
    UNCHECKED = 1
    SUCCESS = 2
    FAILURE = 3


class Firing(QObject):
    newGraph = pyqtSignal(object)
    newSetupPacket = pyqtSignal(object)
    newErrorPacket = pyqtSignal(object)
    newFiringPacket = pyqtSignal(object)

    fullSizeKnown = pyqtSignal(int)
    newResultsPacket = pyqtSignal()
    initialResultsTime = pyqtSignal(float)

    fired = pyqtSignal()
    stopped = pyqtSignal()
    hasResults = pyqtSignal()

    def __init__(self, forceConverter, pressureConverter, motorInfo, port):
        super().__init__()
        self.versionChecked = VERSION_CHECK_STATE.UNCHECKED
        self._exiting = False

        self.lastFiringPacket = None
        self.firingPackets = []

        self.rawData = {}
        self.startIndex = None
        self.lastSend = 0
        self.lastSequenceMod = None
        self.fullSize = None
        self.firing = False
        self.onResultsView = False

        self.forceConverter = forceConverter
        self.pressureConverter = pressureConverter
        self.motorInfo = motorInfo

        self.radioManager = RadioManager()
        self.radioManager.newPacket.connect(self.newPacket)
        self.radioManager.run(port)

        Thread(target=self._backupThread).start()

    def processAndSend(self):
        logger.log('Processing more data ({}->{})'.format(self.lastSend, len(self.rawData)))
        raw = {'time': [], 'force': [], 'pressure': []}

        recv = list(self.rawData.keys())
        recv.sort()
        for i in recv:
            raw['time'].append(self.rawData[i].time)
            raw['force'].append(self.rawData[i].force)
            raw['pressure'].append(self.rawData[i].pressure)

        # Adjust for 16 bit time rolling over. This logic is duplicated from motorDataWidget.py
        minTime = 0
        for i, time in enumerate(raw['time']):
            if i > 0 and time + minTime < raw['time'][i - 1]:
                minTime += 2 ** 16
            raw['time'][i] = time + minTime

        self.lastSend = len(self.rawData)

        self.newGraph.emit(processRawData(raw, self.forceConverter, self.pressureConverter, self.motorInfo))

    def newPacket(self, packet):
        if type(packet) is VersionPacket and self.versionChecked == VERSION_CHECK_STATE.UNCHECKED:
            if checkVersionPacket(packet):
                logger.log('Board version check passed ({})'.format(packet))
                self.versionChecked = VERSION_CHECK_STATE.SUCCESS
            else:
                logger.error('Board version check failed ({})'.format(packet))
                self.versionChecked = VERSION_CHECK_STATE.FAILURE
            return
        if not self.versionChecked == VERSION_CHECK_STATE.SUCCESS:
            # Don't process any packets until we are sure we know how to talk to this board
            return
        if type(packet) is SetupPacket:
            self.newSetupPacket.emit(packet)
        elif type(packet) is ErrorPacket:
            self.newErrorPacket.emit(packet)
        elif type(packet) is FiringPacket:
            self.newFiringPacket.emit(packet)
            if self.lastFiringPacket is None or packet.time != self.lastFiringPacket.time:
                self.lastFiringPacket = packet
                self.firingPackets.append(packet)
        elif type(packet) is ResultPacket:
            self.newResultsPacket.emit()
            if not packet.validate():
                logger.warn('Invalid results packet, {}'.format(packet))
                return
            self.rawData[packet.seqNum] = packet
            if len(self.rawData) == 1:
                logger.log('Got first result packet, setting start index to {}'.format(packet.seqNum))
                self.startIndex = packet.seqNum
            else:
                if self.lastSend == 0:
                    self.initialResultsTime.emit(len(self.rawData) / 15)
                    if abs(packet.seqNum - self.startIndex) < PACKET_STRIDE:
                        logger.log('Latest seq num ({}) close to start index ({})'.format(packet.seqNum, self.startIndex))
                        # The number of datapoints in a recording is always a multiple of 64 so we can figure out the
                        # size of the recording from the partial data assuming we got one of the last 64 datapoints.
                        # If not, it isn't a big deal because only the progress bar is impacted.
                        self.fullSize = ceil(max(self.rawData.keys()) / 64) * 64
                        self.fullSizeKnown.emit(self.fullSize)
                        self.lastSequenceMod = packet.seqNum % PACKET_STRIDE
                        self.processAndSend()
                        self.onResultsView = True

                diffSeqMod = self.lastSequenceMod is not None and packet.seqNum % PACKET_STRIDE != self.lastSequenceMod
                if diffSeqMod and len(self.rawData) > self.lastSend and self.motorInfo is not None:
                    self.lastSequenceMod = packet.seqNum % PACKET_STRIDE
                    self.processAndSend()
                    if self.lastSend == self.fullSize:
                        logger.log('Done receiving data')
                        self.radioManager.stop()

    def fire(self):
        if not self.versionChecked:
            # This method should never be called if a version check hasn't passed, but just in case...
            logger.error('Attempted to fire without version check!')
            return
        firingDur = int(self.motorInfo.getProperty('firingDuration') * 1000)
        firePack = FirePacket(firingDur)
        self.radioManager.sendPacket(firePack, 50)
        self.fired.emit()
        self.firing = True
        Thread(target=self._fireThread).start()

    def stopFiring(self):
        self.firing = False
        self.radioManager.clearSendBuffer()

    def _fireThread(self):
        firingDur = int(self.motorInfo.getProperty('firingDuration') * 1000)
        firePack = FirePacket(firingDur)
        while self.firing and not self._exiting:
            self.radioManager.sendPacket(firePack, 5)
            sleep(0.05)

    def stop(self):
        Thread(target=self._stopThread).start()

    def _stopThread(self):
        stopPack = StopPacket()
        while self.startIndex is None and not self._exiting:
            self.radioManager.sendPacket(stopPack, 5)
            sleep(0.1)
        self.radioManager.clearSendBuffer()
        self.stopped.emit()

    def _backupThread(self):
        dataLine = '\nT:{},F:{},P:{}'
        backupFilePath = '{}/{}'.format(fileIO.getDataDirectory(), 'firings.bak')
        try:
            with open(backupFilePath, 'a') as backupFile:
                backupFile.write('\n{} Firing {}'.format('#' * 30, '#' * 30))
                backupFile.write('\nMotor info: {}'.format(self.motorInfo.getProperties()))
                forceProps = self.forceConverter.getProperties() if self.forceConverter is not None else None
                backupFile.write('\nLoad Cell: {}'.format(forceProps))
                pressProps = self.pressureConverter.getProperties() if self.pressureConverter is not None else None
                backupFile.write('\nPressure Transducer: {}'.format(pressProps))
                backupFile.write('\nData:')
                while not self._exiting:
                    sleep(0.05)
                    while len(self.firingPackets) > 0:
                        packet = self.firingPackets.pop(0)
                        backupFile.write(dataLine.format(packet.time, packet.force, packet.pressure))
        except:
            logger.error('Failed to open firing data backup file at ({})'.format(backupFilePath))

    def exit(self):
        self._exiting = True
        self.radioManager.stop()
