import Universe
import wx
import copy

########################################################################################
#"Picker" classes
########################################################################################

class DestinationPicker(wx.Dialog):
    #fields
    universe = []
    destination1 = 0
    destination2 = 0

    def __init__(self, parent, id, universe, destination1, destination2):

        wx.Dialog.__init__(self, parent, id)
        self.SetTitle("Destination Picker")

        self.universe = universe

        self.lblDestination1 = wx.StaticText(self, -1, "Starting location")
        self.lblDestination2 = wx.StaticText(self, -1, "Final location")
        self.Destination1Picker = wx.ListBox(self, -1, choices=self.universe.destination_menu, size=(300,-1), style=wx.LB_SINGLE)
        self.Destination2Picker = wx.ListBox(self, -1, choices=self.universe.destination_menu, size=(300,-1), style=wx.LB_SINGLE)

        self.btnOK = wx.Button(self, -1, "OK")
        self.btnCancel = wx.Button(self, -1, "Cancel")

        maingrid = wx.FlexGridSizer(2, 2, 5, 5)
        
        maingrid.AddMany([self.lblDestination1, self.lblDestination2,
                          self.Destination1Picker, self.Destination2Picker,
                          self.btnOK, self.btnCancel])

        self.SetSizerAndFit(maingrid)

        if destination1 == -1:
            destination1 = 0
        if destination2 == -1:
            destination2 = 0

        self.destination1 = destination1
        self.destination2 = destination2
        self.Destination1Picker.SetSelection(destination1)
        self.Destination2Picker.SetSelection(destination2)

        font = self.GetFont()
        font.SetWeight(wx.FONTWEIGHT_BOLD)
        self.lblDestination1.SetFont(font)
        self.lblDestination2.SetFont(font)

        self.btnOK.Bind(wx.EVT_BUTTON, self.ClickOK)
        self.btnCancel.Bind(wx.EVT_BUTTON, self.ClickCancel)

    def ClickOK(self, e):
        self.destination1 = self.Destination1Picker.GetSelection()
        self.destination2 = self.Destination2Picker.GetSelection()
        
        if self.destination1 == 0:
            self.destination1 = -1
        if self.destination2 == 0:
            self.destination2 = -1

        self.Close()

    def ClickCancel(self, e):
        if self.destination1 == 0:
            self.destination1 = -1
        if self.destination2 == 0:
            self.destination2 = -1

        self.Close()


class SequencePicker(wx.Dialog):
    #fields
    universe = []
    missionoptions = []
    ActiveJourney = []
    ActiveSequence = []
    ActivePhase = []
    ListOfSequences = []
    CurrentSequence = []
    SaveOldSequence = []

    def __init__(self, parent, id, universe, missionoptions, ActiveJourney):

        wx.Dialog.__init__(self, parent, id)

        self.SetSize((800,500))
        self.SetTitle("Flyby Picker")

        self.universe = universe

        self.lblAvailableBodies = wx.StaticText(self, -1, "Available flyby bodies")
        self.lblChooseSequence = wx.StaticText(self, -1, "Select a sequence")
        self.lblCurrentSequence = wx.StaticText(self, -1, "Current sequence")
        font = self.GetFont()
        font.SetWeight(wx.FONTWEIGHT_BOLD)
        self.lblAvailableBodies.SetFont(font)
        self.lblChooseSequence.SetFont(font)
        self.lblCurrentSequence.SetFont(font)

        self.universe = universe
        self.missionoptions = missionoptions
        self.ActiveJourney = ActiveJourney
        self.ActiveSequence = 0
        self.ActivePhase = 0
        self.SaveOldSequence = copy.deepcopy(self.missionoptions.Journeys[self.ActiveJourney].sequence)
        
        self.AvailableBodiesPicker = wx.ListBox(self, -1, choices=self.universe.flyby_menu, size=(300,-1), style=wx.LB_SINGLE)
        
        self.UpdateLists()

        self.SequencePicker = wx.ListBox(self, -1, choices=self.ListOfSequences, size=(500,-1), style=wx.LB_SINGLE)

        self.CurrentSequencePicker = wx.ListBox(self, -1, choices=self.CurrentSequence, size=(500,300), style=wx.LB_SINGLE)

        self.btnAddNewTrialSequence = wx.Button(self, -1, "Add new trial sequence")
        self.btnRemoveTrialSequence = wx.Button(self, -1, "Remove trial sequence", size=self.btnAddNewTrialSequence.GetSize())
        self.btnAddFlyby = wx.Button(self, -1, "Add flyby", size=self.btnAddNewTrialSequence.GetSize())
        self.btnRemoveFlyby = wx.Button(self, -1, "Remove flyby", size=self.btnAddNewTrialSequence.GetSize())
        self.btnMoveFlybyUp = wx.Button(self, -1, "Move flyby up", size=self.btnAddNewTrialSequence.GetSize())
        self.btnMoveFlybyDown = wx.Button(self, -1, "Move flyby down", size=self.btnAddNewTrialSequence.GetSize())
        buttongrid = wx.GridSizer(6,1,0,0)
        buttongrid.AddMany([self.btnAddNewTrialSequence, self.btnRemoveTrialSequence, self.btnAddFlyby, self.btnRemoveFlyby, self.btnMoveFlybyUp, self.btnMoveFlybyDown])

        selectorstacker = wx.BoxSizer(wx.VERTICAL)
        selectorstacker.AddMany([self.lblChooseSequence, self.SequencePicker, self.lblCurrentSequence, self.CurrentSequencePicker])

        availablebodiesstacker = wx.BoxSizer(wx.VERTICAL)
        availablebodiesstacker.AddMany([self.lblAvailableBodies, self.AvailableBodiesPicker])

        pickerbox = wx.BoxSizer(wx.HORIZONTAL)
        pickerbox.AddMany([availablebodiesstacker, buttongrid, selectorstacker])

        self.btnOK = wx.Button(self, -1, "OK")
        self.btnCancel = wx.Button(self, -1, "Cancel")

        bottombox = wx.BoxSizer(wx.HORIZONTAL)
        bottombox.AddMany([self.btnOK, self.btnCancel])

        mainbox = wx.BoxSizer(wx.VERTICAL)
        
        mainbox.AddMany([pickerbox, bottombox])

        self.btnOK.Bind(wx.EVT_BUTTON, self.ClickOK)
        self.btnCancel.Bind(wx.EVT_BUTTON, self.ClickCancel)
        self.btnAddNewTrialSequence.Bind(wx.EVT_BUTTON, self.ClickAddNewTrialSequence)
        self.btnRemoveTrialSequence.Bind(wx.EVT_BUTTON, self.ClickRemoveTrialSequence)
        self.btnAddFlyby.Bind(wx.EVT_BUTTON, self.ClickAddFlyby)
        self.btnRemoveFlyby.Bind(wx.EVT_BUTTON, self.ClickRemoveFlyby)
        self.btnMoveFlybyUp.Bind(wx.EVT_BUTTON, self.ClickMoveFlybyUp)
        self.btnMoveFlybyDown.Bind(wx.EVT_BUTTON, self.ClickMoveFlybyDown)
        self.SequencePicker.Bind(wx.EVT_LISTBOX, self.PickDifferentSequence)

        self.AvailableBodiesPicker.SetSelection(0)
        self.SequencePicker.SetSelection(self.ActiveSequence)
        if len(self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence]) > 0:
            self.CurrentSequencePicker.SetSelection(self.ActivePhase)
        self.SetSizerAndFit(mainbox)

    def UpdateLists(self):
        self.ListOfSequences = []
        self.CurrentSequence = []

        for seq in range(0, len(self.missionoptions.Journeys[self.ActiveJourney].sequence)):
            ThisSequence = []
            while 0 in self.missionoptions.Journeys[self.ActiveJourney].sequence[seq]:
                self.missionoptions.Journeys[self.ActiveJourney].sequence[seq].remove(0)
            for b in self.missionoptions.Journeys[self.ActiveJourney].sequence[seq]:
                if b > 0:
                    ThisSequence.append(self.universe.flyby_menu[b-1])
            if ThisSequence == []:
                self.ListOfSequences.append("No flybys")
            else:
                self.ListOfSequences.append(str(ThisSequence))
    
        if len(self.missionoptions.Journeys[self.ActiveJourney].sequence) > 0:
            for b in self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence]:
                if b > 0:
                    self.CurrentSequence.append(self.universe.flyby_menu[b-1])

    def UpdateListBoxes(self):
        self.SequencePicker.SetItems(self.ListOfSequences)
        self.CurrentSequencePicker.SetItems(self.CurrentSequence)

        self.SequencePicker.SetSelection(self.ActiveSequence)

    def ClickOK(self, e):
        #save current information

        #first we need to increate max_phases_per_journey to match the longest sequence
        for seq in range(0, len(self.missionoptions.Journeys[self.ActiveJourney].sequence)):
            if self.missionoptions.max_phases_per_journey < len(self.missionoptions.Journeys[self.ActiveJourney].sequence[seq]):
                self.missionoptions.max_phases_per_journey = len(self.missionoptions.Journeys[self.ActiveJourney].sequence[seq])

        #then we need to pad all "short" sequences in all journeys with trailing zeros
        for j in range(0, self.missionoptions.number_of_journeys):
            for seq in range(0, len(self.missionoptions.Journeys[j].sequence)):
                while len(self.missionoptions.Journeys[j].sequence[seq]) < self.missionoptions.max_phases_per_journey:
                    self.missionoptions.Journeys[j].sequence[seq].append(0)

        #the number of trial sequences for the mission must equal the number of trial sequences entered here
        #similarly we must add or remove trial decision vectors
        #if this journey has too many trial sequences, add additional sequences to the other journeys
        if len(self.missionoptions.Journeys[self.ActiveJourney].sequence) > self.missionoptions.number_of_trial_sequences:
            #sequences
            self.missionoptions.number_of_trial_sequences = len(self.missionoptions.Journeys[self.ActiveJourney].sequence)
            for j in range(0, self.missionoptions.number_of_journeys):
                if j != self.ActiveJourney:
                    while len(self.missionoptions.Journeys[j].sequence) < self.missionoptions.number_of_trial_sequences:
                        self.missionoptions.Journeys[j].sequence.append([0]*self.missionoptions.max_phases_per_journey)

            #trial decision vectors
            while len(self.missionoptions.trialX) < self.missionoptions.number_of_trial_sequences:
                self.missionoptions.trialX.append([0.0])
            while len(self.missionoptions.trialX) > self.missionoptions.number_of_trial_sequences:
                self.missionoptions.trialX.pop()

        #if this journey has fewer sequences than the others, truncate the sequence lists of the other journeys
        elif len(self.missionoptions.Journeys[self.ActiveJourney].sequence) < self.missionoptions.number_of_trial_sequences:
            #sequences
            self.missionoptions.number_of_trial_sequences = len(self.missionoptions.Journeys[self.ActiveJourney].sequence)
            for j in range(0, self.missionoptions.number_of_journeys):
                if j != self.ActiveJourney:
                    while len(self.missionoptions.Journeys[j].sequence) > self.missionoptions.number_of_trial_sequences:
                        self.missionoptions.Journeys[j].sequence.pop()

            #trial decision vectors
            while len(self.missionoptions.trialX) < self.missionoptions.number_of_trial_sequences:
                self.missionoptions.trialX.append([0.0])
            while len(self.missionoptions.trialX) > self.missionoptions.number_of_trial_sequences:
                self.missionoptions.trialX.pop()

        self.Close()

    def ClickCancel(self, e):
        #restore old information
        self.missionoptions.Journeys[self.ActiveJourney].sequence = copy.deepcopy(self.SaveOldSequence)
        self.Close()

    def ClickAddNewTrialSequence(self, e):
        self.missionoptions.Journeys[self.ActiveJourney].sequence.append([0]*self.missionoptions.max_phases_per_journey)
        self.UpdateLists()
        self.UpdateListBoxes()
        self.ActiveSequence = len(self.missionoptions.Journeys[self.ActiveJourney].sequence) - 1

    def ClickRemoveTrialSequence(self, e):
        if len(self.missionoptions.Journeys[self.ActiveJourney].sequence) > 1:
            self.ActiveSequence = self.SequencePicker.GetSelection()
            self.missionoptions.Journeys[self.ActiveJourney].sequence.pop(self.ActiveSequence)

            if self.ActiveSequence > len(self.missionoptions.Journeys[self.ActiveJourney].sequence) - 1:
                self.ActiveSequence -= 1

            self.UpdateLists()
            self.UpdateListBoxes()

            self.SequencePicker.SetSelection(self.ActiveSequence)

    def PickDifferentSequence(self, e):
        self.ActiveSequence = self.SequencePicker.GetSelection()
        self.UpdateLists()
        self.UpdateListBoxes()
        self.ActivePhase = 0
        if len(self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence]) > 0:
            self.CurrentSequencePicker.SetSelection(self.ActivePhase)

    def ClickAddFlyby(self, e):
        BodyChoice = self.AvailableBodiesPicker.GetSelection() + 1
        if BodyChoice == 0:
            BodyChoice = 1

        self.ActiveSequence = self.SequencePicker.GetSelection()
        self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence].append(BodyChoice)

        self.UpdateLists()
        self.UpdateListBoxes()

    def ClickRemoveFlyby(self, e):
        self.ActivePhase = self.CurrentSequencePicker.GetSelection()
        self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence].pop(self.ActivePhase)
        self.UpdateLists()
        self.UpdateListBoxes()

    def ClickMoveFlybyUp(self, e):
        self.ActivePhase = self.CurrentSequencePicker.GetSelection()
        self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence][self.ActivePhase], self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence][self.ActivePhase - 1] = self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence][self.ActivePhase - 1], self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence][self.ActivePhase]

        if self.ActivePhase > 0:
            self.missionoptions.Journeys[self.missionoptions.ActiveJourney], self.missionoptions.Journeys[self.missionoptions.ActiveJourney-1] = self.missionoptions.Journeys[self.missionoptions.ActiveJourney-1], self.missionoptions.Journeys[self.missionoptions.ActiveJourney]
            self.ActivePhase = 1
        
        self.UpdateLists()
        self.UpdateListBoxes()
        self.CurrentSequencePicker.SetSelection(self.ActivePhase)

    def ClickMoveFlybyDown(self, e):
        self.ActivePhase = self.CurrentSequencePicker.GetSelection()

        if self.ActivePhase < len(self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence]):
            self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence][self.ActivePhase], self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence][self.ActivePhase + 1] = self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence][self.ActivePhase + 1], self.missionoptions.Journeys[self.ActiveJourney].sequence[self.ActiveSequence][self.ActivePhase]
            self.ActivePhase += 1

        self.UpdateLists()
        self.UpdateListBoxes()
        self.CurrentSequencePicker.SetSelection(self.ActivePhase)