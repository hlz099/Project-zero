import Journey
import MissionEvent
import os
import math

import numpy as np
from scipy.integrate import ode
import matplotlib
import pylab
from mpl_toolkits.mplot3d import Axes3D

class Mission(object):
    #class to hold mission information

    def __init__(self, input_file_name):
       self.parse_mission_file(input_file_name)

    def parse_mission_file(self, input_file_name):
        #Step 1: open the file
        if os.path.isfile(input_file_name):
            inputfile = open(input_file_name, "r")
            self.success = 1
        else:
            dlg = wx.MessageDialog(self, "Unable to open" + input_file_name, "EMTG Error", wx.ID_OK)
            dlg.ShowModal()
            dlg.Destroy()
            self.success = 0
            return

        #Step 2: parse the file
        self.ActiveJourney = -1
        self.Journeys = []
        linenumber = 0
        for line in inputfile:
            linenumber += 1
            #split the line by colons
            linecell = line.split(':')

            if linecell[0] == "Mission":
                self.mission_name = linecell[1]

            elif linecell[0] == "Decision Vector":
                DecisionCell = linecell[1].split(' ')
                DecisionCell.remove('')
                self.DecisionVector = []
                for entry in DecisionCell:
                    self.DecisionVector.append(float(entry))

            elif linecell[0] == "Journey":
                self.ActiveJourney = int(linecell[1]) - 1
                self.Journeys.append(Journey.Journey())

            if self.ActiveJourney >= 0:
                if linecell[0] == "Journey name":
                    self.Journeys[self.ActiveJourney].journey_name = linecell[1].strip()
                elif linecell[0] == "Central Body":
                    self.Journeys[self.ActiveJourney].central_body = linecell[1].strip()
                elif linecell[0] == "Thruster duty cycle":
                    self.Journeys[self.ActiveJourney].thruster_duty_cycle = eval(linecell[1])
                elif linecell[0] == "Radius (km)":
                    self.Journeys[self.ActiveJourney].central_body_radius = eval(linecell[1])
                elif linecell[0] == "mu (km^2/s^3)":
                    self.Journeys[self.ActiveJourney].mu = eval(linecell[1])
                elif linecell[0] == "Characteristic length unit (km)":
                    self.Journeys[self.ActiveJourney].LU = eval(linecell[1])
                    self.Journeys[self.ActiveJourney].TU = math.sqrt(self.Journeys[self.ActiveJourney].LU**3 / self.Journeys[self.ActiveJourney].mu)
                elif linecell[0] == "Boundary":
                    boundary_elements = linecell[1].split(' ')
                    boundary_state = [float(boundary_elements[2]), float(boundary_elements[3]), float(boundary_elements[4]), float(boundary_elements[5]), float(boundary_elements[6]), float(boundary_elements[7])]
                    self.Journeys[self.ActiveJourney].boundary_states.append(boundary_state)
                elif linecell[0] == "Flyby":
                    flyby_elements = linecell[1].split(' ')
                    flyby_state = [float(flyby_elements[3]), float(flyby_elements[4]), float(flyby_elements[5]), float(flyby_elements[6]), float(flyby_elements[7]), float(flyby_elements[8])]
                    self.Journeys[self.ActiveJourney].flyby_periapse_states.append(flyby_state)
                elif linecell[0] == "End journey\n":
                    self.ActiveJourney = -1

                else:
                    #parse event lines
                    inputcell = line.split('|')
                    if inputcell[0].strip(' ').isdigit():
                        tempEvent = MissionEvent.MissionEvent(inputcell)
                        self.Journeys[self.ActiveJourney].missionevents.append(tempEvent)
                        del tempEvent
                    del inputcell

        inputfile.close()

    def PlotMission(self, PlotOptions):
        self.MissionFigure = matplotlib.pyplot.figure()
        matplotlib.rcParams.update({'font.size': PlotOptions.FontSize})
        self.MissionFigure.subplots_adjust(left=0.01, right=0.99, bottom=0.01, top=0.99)
        self.MissionAxes = self.MissionFigure.add_axes([0,0,1,1], projection='3d')
        self.MissionAxes.set_aspect('equal')
        self.MissionAxes.set_xlabel('x (km)')
        self.MissionAxes.set_ylabel('y (km)')
        self.MissionAxes.set_zlabel('z (km)')
        self.MissionAxes.autoscale_view(tight=True, scalex=True, scaley=True, scalez=True)
        self.MissionAxes.view_init(elev=90, azim=-90)
        self.MissionAxes.grid(b=False)

        #plot journey boundary orbits
        if PlotOptions.ShowBoundaryOrbits:
            if self.ActiveJourney <= len(self.Journeys) - 1:
                self.Journeys[self.ActiveJourney].PlotJourneyBoundaryOrbits(self.MissionAxes)
            else:
                for CurrentJourney in self.Journeys:
                    CurrentJourney.PlotJourneyBoundaryOrbits(self.MissionAxes)

        if self.ActiveJourney <= len(self.Journeys) - 1:
            self.Journeys[self.ActiveJourney].PlotJourney(self.MissionAxes, PlotOptions)
        else:
            for CurrentJourney in self.Journeys:
                CurrentJourney.PlotJourney(self.MissionAxes, PlotOptions)

        #plot the central body
        u = np.linspace(0, 2 * np.pi, 100)
        v = np.linspace(0, np.pi, 100)

        x = self.Journeys[0].central_body_radius * np.outer(np.cos(u), np.sin(v))
        y = self.Journeys[0].central_body_radius * np.outer(np.sin(u), np.sin(v))
        z = self.Journeys[0].central_body_radius * np.outer(np.ones(np.size(u)), np.cos(v))
        self.MissionAxes.plot_surface(x, y, z,  rstride=10, cstride=10, color='DarkOrange', linewidth=0)

        

        X = self.MissionAxes.get_xlim()
        Y = self.MissionAxes.get_ylim()
        Z = self.MissionAxes.get_zlim()
        
        # Create cubic bounding box to simulate equal aspect ratio
        max_range = np.array([X.max()-X.min(), Y.max()-Y.min(), Z.max()-Z.min()]).max()
        Xb = 0.5*max_range*np.mgrid[-1:2:2,-1:2:2,-1:2:2][0].flatten() + 0.5*(X.max()+X.min())
        Yb = 0.5*max_range*np.mgrid[-1:2:2,-1:2:2,-1:2:2][1].flatten() + 0.5*(Y.max()+Y.min())
        Zb = 0.5*max_range*np.mgrid[-1:2:2,-1:2:2,-1:2:2][2].flatten() + 0.5*(Z.max()+Z.min())
        # Comment or uncomment following both lines to test the fake bounding box:
        for xb, yb, zb in zip(Xb, Yb, Zb):
           self.MissionAxes.plot([xb], [yb], [zb], 'w')

        if PlotOptions.ShowTextDescriptions:
            self.MissionFigure.canvas.mpl_connect('button_release_event', self.UpdateLabelPositionsEvent)

        self.MissionFigure.show()

        #these lines have to be run AFTER MissionFigure.show() is called
        if PlotOptions.ShowTextDescriptions:
            self.UpdateLabelPositions()

    def UpdateLabelPositionsEvent(self, e):
        self.UpdateLabelPositions()

    def UpdateLabelPositions(self):
        if self.ActiveJourney <= len(self.Journeys) - 1:
            self.Journeys[self.ActiveJourney].UpdateLabelPosition(self.MissionFigure, self.MissionAxes)
        else:
            for CurrentJourney in self.Journeys:
                CurrentJourney.UpdateLabelPosition(self.MissionFigure, self.MissionAxes)

    def GenerateDataPlot(self, PlotOptions):
        if PlotOptions.PlotR or PlotOptions.PlotV or PlotOptions.PlotThrust or PlotOptions.PlotIsp or PlotOptions.PlotMdot or PlotOptions.PlotEfficiency or PlotOptions.PlotThrottle or PlotOptions.PlotPower or PlotOptions.PlotGamma or PlotOptions.PlotDelta or PlotOptions.PlotCB_thrust_angle or PlotOptions.PlotMass or PlotOptions.PlotNumberOfEngines or PlotOptions.PlotActivePower or PlotOptions.PlotWasteHeat:
            self.DataFigure = matplotlib.pyplot.figure()
            self.DataAxes = self.DataFigure.add_axes([0.1, 0.1, 0.8, 0.8])
            matplotlib.rcParams.update({'font.size': PlotOptions.FontSize})

            if self.ActiveJourney <= len(self.Journeys) - 1:
                self.Journeys[self.ActiveJourney].GenerateJourneyDataPlot(self.DataAxes, PlotOptions, True)

                if PlotOptions.PlotCriticalEvents:
                    self.Journeys[self.ActiveJourney].PlotPhaseBoundariesOnDataPlot(self.DataAxes, PlotOptions, True)
            else:
                for j in range(0, len(self.Journeys)):
                    if j == 0:
                        self.Journeys[j].GenerateJourneyDataPlot(self.DataAxes, PlotOptions, True)
                        if PlotOptions.PlotCriticalEvents:
                            self.Journeys[j].PlotPhaseBoundariesOnDataPlot(self.DataAxes, PlotOptions, True)
                    else:
                        self.Journeys[j].GenerateJourneyDataPlot(self.DataAxes, PlotOptions, False)
                        if PlotOptions.PlotCriticalEvents:
                            self.Journeys[j].PlotPhaseBoundariesOnDataPlot(self.DataAxes, PlotOptions, False)

            self.DataAxes.set_xlabel('Epoch')
            def format_date(x, pos=None):
                return pylab.num2date(x).strftime('%m-%d-%Y')

            self.DataAxes.xaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(format_date))
            self.DataFigure.autofmt_xdate()
            leg = self.DataAxes.legend(loc='best', fancybox=True)
            leg.get_frame().set_alpha(0.5)
            leg.draggable(use_blit=True)
            self.DataFigure.show()

    def OutputSTKEphemeris(self, MissionPanel):
        if self.ActiveJourney <= len(self.Journeys) - 1:
            self.Journeys[self.ActiveJourney].OutputSTKEphemeris(MissionPanel)
        else:
            for CurrentJourney in self.Journeys:
                CurrentJourney.OutputSTKEphemeris(MissionPanel)