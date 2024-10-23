from PyQt6.QtWidgets import QLabel
from PyQt6.QtCore import pyqtSignal

from motorlib.units import convert
from motorlib.propellant import PropellantTab
from motorlib.constants import gasConstant

from .collectionEditor import CollectionEditor

class PropellantTabEditor(CollectionEditor):

    modified = pyqtSignal()

    def __init__(self, parent):
        super().__init__(parent, False)

        self.labelCStar = QLabel("Characteristic Velocity: -")
        self.labelCStar.hide()
        self.stats.addWidget(self.labelCStar)

    def propertyUpdate(self):
        k = self.propertyEditors['k'].getValue()
        t = self.propertyEditors['t'].getValue()
        m = self.propertyEditors['m'].getValue()
        num = (k * gasConstant / m * t) ** 0.5
        denom = k * ((2 / (k + 1)) ** ((k + 1) / (k - 1))) ** 0.5
        charVel = num / denom

        if self.preferences is not None:
            dispUnit = self.preferences.getUnit('m/s')
        else:
            dispUnit = 'm/s'

        cStarText = '{} {}'.format(int(convert(charVel, 'm/s', dispUnit)), dispUnit)

        self.labelCStar.setText('Characteristic Velocity: {}'.format(cStarText))
        self.modified.emit()

    def cleanup(self):
        self.labelCStar.hide()
        super().cleanup()

    def getProperties(self): # Override to change units on ballistic coefficient
        res = super().getProperties()
        coeffUnit = self.propertyEditors['a'].dispUnit
        if coeffUnit == 'in/(s*psi^n)':
            res['a'] *= 1 / (6895 ** res['n'])
        return res

    def loadProperties(self, obj): # Override for ballistic coefficient units
        props = obj.getProperties()
        # Convert the ballistic coefficient based on the exponent
        ballisticCoeffUnit = self.preferences.getUnit('m/(s*Pa^n)')
        if ballisticCoeffUnit == 'in/(s*psi^n)':
            props['a'] /= 1 / (6895 ** props['n'])
        # Create a new propellant instance using the new A
        newPropTab = PropellantTab()
        newPropTab.setProperties(props)
        super().loadProperties(newPropTab)
        self.labelCStar.show()
