"""D Grain submodule"""

from ..grain import FmmGrain
from ..properties import FloatProperty
from ..simResult import SimAlert, SimAlertLevel, SimAlertType

class DGrain(FmmGrain):
    """Defines a D grain, which is a grain that has no propellant past a chord that is a user-specified distance from
    the diameter."""
    geomName = 'D Grain'
    def __init__(self):
        super().__init__()
        self.props['slotOffset'] = FloatProperty('Slot offset', 'm', -1, 1)

        self.props['slotOffset'].setValue(0)

    def generateCoreMap(self):
        slotOffset = self.normalize(self.props['slotOffset'].getValue())

        self.coreMap[self.mapX > slotOffset] = 0

    def getDetailsString(self, lengthUnit='m'):
        return 'Length: {}, Slot offset: {}'.format(self.props['length'].dispFormat(lengthUnit),
                                                    self.props['slotOffset'].dispFormat(lengthUnit))

    def getGeometryErrors(self):
        errors = super().getGeometryErrors()

        if self.props['slotOffset'].getValue() > self.props['diameter'].getValue() / 2:
            aText = 'Core offset must not be greater than grain radius'
            errors.append(SimAlert(SimAlertLevel.ERROR, SimAlertType.GEOMETRY, aText))
        if self.props['slotOffset'].getValue() < -self.props['diameter'].getValue() / 2:
            aText = 'Core offset must be greater than negative grain radius'
            errors.append(SimAlert(SimAlertLevel.ERROR, SimAlertType.GEOMETRY, aText))

        return errors
