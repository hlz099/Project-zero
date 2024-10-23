import wx
import Universe

class UniverseNotebook(wx.Notebook):
    #class for Universe notebook
    universe = []

    def __init__(self, parent, universe):
        wx.Notebook.__init__(self, parent=parent, id=wx.ID_ANY, style=
                             wx.BK_DEFAULT, size=(800,600)
                             )
                             
        font = self.GetFont()
        font.SetPointSize(10)
        self.SetFont(font)

        self.universe = universe

        #create tabs
        self.tabUniverse = UniversePanel(self, universe)
        self.AddPage(self.tabUniverse, "Universe Properties")
        self.tabBodies = BodyPanel(self, universe)
        self.AddPage(self.tabBodies, "Universe Body Properties")

class UniversePanel(wx.Panel):
    def __init__(self, parent, universe):

        wx.Panel.__init__(self, parent)

        self.universe = universe

        self.lblcentral_body_name = wx.StaticText(self, -1, "Universe name")
        self.lblcentral_body_SPICE_ID = wx.StaticText(self, -1, "Central body SPICE ID")
        self.lblcentral_body_radius = wx.StaticText(self, -1, "Central body radius (km)")
        self.lblmu = wx.StaticText(self, -1, "mu (km^3/s^2)")
        self.lblLU = wx.StaticText(self, -1, "characteristic length unit (km)")
        self.lblr_SOI = wx.StaticText(self, -1, "sphere of influence radius (km)")
        self.lblminimum_safe_distance = wx.StaticText(self, -1, "minimum safe distance from origin (km)")
        self.lblalpha0 = wx.StaticText(self, -1, "alpha0")
        self.lblalphadot = wx.StaticText(self, -1, "alphadot")
        self.lbldelta0 = wx.StaticText(self, -1, "delta0")
        self.lbldeltadot = wx.StaticText(self, -1, "deltadot")
        self.lblW0 = wx.StaticText(self, -1, "W0")
        self.lblWdot = wx.StaticText(self, -1, "Wdot")

        self.txtcentral_body_name = wx.TextCtrl(self, -1, "Universe name", size = (300,-1))
        self.txtcentral_body_SPICE_ID = wx.TextCtrl(self, -1, "Central body SPICE ID", size = (300,-1))
        self.txtcentral_body_radius = wx.TextCtrl(self, -1, "Central body radius (km)", size = (300,-1))
        self.txtmu = wx.TextCtrl(self, -1, "mu (km^3/s^2)", size = (300,-1))
        self.txtLU = wx.TextCtrl(self, -1, "characteristic length unit (km)", size = (300,-1))
        self.txtr_SOI = wx.TextCtrl(self, -1, "sphere of influence radius (km)", size = (300,-1))
        self.txtminimum_safe_distance = wx.TextCtrl(self, -1, "minimum safe distance from origin (km)", size = (300,-1))
        self.txtalpha0 = wx.TextCtrl(self, -1, "alpha0", size = (300,-1))
        self.txtalphadot = wx.TextCtrl(self, -1, "alphadot", size = (300,-1))
        self.txtdelta0 = wx.TextCtrl(self, -1, "delta0", size = (300,-1))
        self.txtdeltadot = wx.TextCtrl(self, -1, "deltadot", size = (300,-1))
        self.txtW0 = wx.TextCtrl(self, -1, "W0", size = (300,-1))
        self.txtWdot = wx.TextCtrl(self, -1, "Wdot", size = (300,-1))

        maingrid = wx.FlexGridSizer(7, 2, 5, 5)
        maingrid.SetFlexibleDirection(wx.VERTICAL)
        maingrid.AddMany([self.lblcentral_body_name, self.txtcentral_body_name,
                          self.lblcentral_body_SPICE_ID, self.txtcentral_body_SPICE_ID,
                          self.lblcentral_body_radius, self.txtcentral_body_radius,
                          self.lblmu, self.txtmu,
                          self.lblLU, self.txtLU,
                          self.lblr_SOI, self.txtr_SOI,
                          self.lblminimum_safe_distance, self.txtminimum_safe_distance,
                          self.lblalpha0, self.txtalpha0,
                          self.lblalphadot, self.txtalphadot,
                          self.lbldelta0, self.txtdelta0,
                          self.lbldeltadot, self.txtdeltadot,
                          self.lblW0, self.txtW0,
                          self.lblWdot, self.txtWdot])

        #bind text fields to functions
        self.txtcentral_body_name.Bind(wx.EVT_KILL_FOCUS, self.Changecentralbodyname)
        self.txtcentral_body_SPICE_ID.Bind(wx.EVT_KILL_FOCUS, self.Changecentral_body_SPICE_ID)
        self.txtcentral_body_radius.Bind(wx.EVT_KILL_FOCUS, self.Changecentral_body_radius)
        self.txtmu.Bind(wx.EVT_KILL_FOCUS, self.Changemu)
        self.txtLU.Bind(wx.EVT_KILL_FOCUS, self.ChangeLU)
        self.txtr_SOI.Bind(wx.EVT_KILL_FOCUS, self.Changer_SOI)
        self.txtminimum_safe_distance.Bind(wx.EVT_KILL_FOCUS, self.Changeminimum_safe_distance)
        self.txtalpha0.Bind(wx.EVT_KILL_FOCUS, self.Changealpha0)
        self.txtalphadot.Bind(wx.EVT_KILL_FOCUS, self.Changealphadot)
        self.txtdelta0.Bind(wx.EVT_KILL_FOCUS, self.Changedelta0)
        self.txtdeltadot.Bind(wx.EVT_KILL_FOCUS, self.Changedeltadot)
        self.txtW0.Bind(wx.EVT_KILL_FOCUS, self.ChangeW0)
        self.txtWdot.Bind(wx.EVT_KILL_FOCUS, self.ChangeWdot)

        #populate all fields
        self.UpdateUniversePanel()

        self.SetSizer(maingrid)

    def UpdateUniversePanel(self):
        self.txtcentral_body_name.SetValue(self.universe.central_body_name)
        self.txtcentral_body_SPICE_ID.SetValue(str(self.universe.central_body_SPICE_ID))
        self.txtcentral_body_radius.SetValue(str(self.universe.central_body_radius))
        self.txtmu.SetValue(str(self.universe.mu))
        self.txtLU.SetValue(str(self.universe.LU))
        self.txtr_SOI.SetValue(str(self.universe.r_SOI))
        self.txtminimum_safe_distance.SetValue(str(self.universe.minimum_safe_distance))
        self.txtalpha0.SetValue(str(self.universe.reference_angles[0]))
        self.txtalphadot.SetValue(str(self.universe.reference_angles[1]))
        self.txtdelta0.SetValue(str(self.universe.reference_angles[2]))
        self.txtdeltadot.SetValue(str(self.universe.reference_angles[3]))
        self.txtW0.SetValue(str(self.universe.reference_angles[4]))
        self.txtWdot.SetValue(str(self.universe.reference_angles[5]))

    def Changecentralbodyname(self, e):
        self.universe.central_body_name = self.txtcentral_body_name.GetValue()

    def Changecentral_body_SPICE_ID(self, e):
        self.universe.central_body_SPICE_ID = eval(self.txtcentral_body_SPICE_ID.GetValue())

    def Changecentral_body_radius(self, e):
        self.universe.central_body_radius = float(self.txtcentral_body_radius.GetValue())

    def Changemu(self, e):
        self.universe.mu = float(self.txtmu.GetValue())

    def ChangeLU(self, e):
        self.universe.LU = float(self.txtLU.GetValue())

    def Changer_SOI(self, e):
        self.universe.r_SOI = float(self.txtr_SOI.GetValue())

    def Changeminimum_safe_distance(self, e):
        self.universe.minimum_safe_distance = float(self.txtminimum_safe_distance.GetValue())

    def Changealpha0(self, e):
        self.universe.reference_angles[0] = float(self.txtalpha0.GetValue())


    def Changealphadot(self, e):
        self.universe.reference_angles[1] = float(self.txtalphadot.GetValue())

    def Changedelta0(self, e):
        self.universe.reference_angles[2] = float(self.txtdelta0.GetValue())

    def Changedeltadot(self, e):
        self.universe.reference_angles[3] = float(self.txtdeltadot.GetValue())

    def ChangeW0(self, e):
        self.universe.reference_angles[4] = float(self.txtW0.GetValue())

    def ChangeWdot(self, e):
        self.universe.reference_angles[5] = float(self.txtWdot.GetValue())

class BodyPanel(wx.Panel):
    def __init__(self, parent, universe):

        self.universe = universe

        wx.Panel.__init__(self, parent)
        
        #update the body list
        self.UpdateBodyList()
        if len(self.bodylist) > 0:
            self.ActiveBody = 0
        else:
            self.ActiveBody = -1

        #listbox of bodies and control buttons
        self.BodiesPicker = wx.ListBox(self, -1, choices=self.bodylist, size=(300,-1), style=wx.LB_SINGLE)
        self.btnAddBody = wx.Button(self, -1, "Add new body")
        self.btnRemoveBody = wx.Button(self, -1, "Remove body")
        self.btnMoveBodyUp = wx.Button(self, -1, "Move body up")
        self.btnMoveBodyDown = wx.Button(self, -1, "Move body down")

        pickerstacker = wx.BoxSizer(wx.VERTICAL)
        pickerstacker.AddMany([self.BodiesPicker, self.btnAddBody, self.btnRemoveBody, self.btnMoveBodyUp, self.btnMoveBodyDown])

        #bindings for list of bodies and control buttons
        self.BodiesPicker.Bind(wx.EVT_LISTBOX, self.ClickBodiesPicker)
        self.btnAddBody.Bind(wx.EVT_BUTTON, self.ClickAddBody)
        self.btnRemoveBody.Bind(wx.EVT_BUTTON, self.ClickRemoveBody)
        self.btnMoveBodyUp.Bind(wx.EVT_BUTTON, self.ClickMoveBodyUp)
        self.btnMoveBodyDown.Bind(wx.EVT_BUTTON, self.ClickMoveBodyDown)

        #body fields
        self.lblname = wx.StaticText(self, -1, "name", size=(300,-1))
        self.lblshortname = wx.StaticText(self, -1, "shortname", size=(300,-1))
        self.lblSPICE_ID = wx.StaticText(self, -1, "SPICE_ID", size=(300,-1))
        self.lblminimum_flyby_altitude = wx.StaticText(self, -1, "minimum flyby altitude", size=(300,-1))
        self.lblmass = wx.StaticText(self, -1, "mass", size=(300,-1))
        self.lblradius = wx.StaticText(self, -1, "radius", size=(300,-1))
        self.lblephemeris_epoch = wx.StaticText(self, -1, "ephemeris_epoch", size=(300,-1))
        self.lblalpha0 = wx.StaticText(self, -1, "alpha0 (deg)", size=(300,-1))
        self.lblalphadot = wx.StaticText(self, -1, "alphadot (deg/century)", size=(300,-1))
        self.lbldelta0 = wx.StaticText(self, -1, "delta0 (deg)", size=(300,-1))
        self.lbldeltadot = wx.StaticText(self, -1, "deltadot (deg/century)", size=(300,-1))
        self.lblW = wx.StaticText(self, -1, "W (deg)", size=(300,-1))
        self.lblWdot = wx.StaticText(self, -1, "Wdot (deg/century)", size=(300,-1))
        self.lblSMA = wx.StaticText(self, -1, "SMA (km)", size=(300,-1))
        self.lblECC = wx.StaticText(self, -1, "ECC", size=(300,-1))
        self.lblINC = wx.StaticText(self, -1, "INC (deg)", size=(300,-1))
        self.lblRAAN = wx.StaticText(self, -1, "RAAN (deg)", size=(300,-1))
        self.lblAOP = wx.StaticText(self, -1, "AOP (deg)", size=(300,-1))
        self.lblMA = wx.StaticText(self, -1, "MA (deg)", size=(300,-1))
        self.txtname = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtshortname = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtSPICE_ID = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtminimum_flyby_altitude = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtmass = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtradius = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtephemeris_epoch = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtalpha0 = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtalphadot = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtdelta0 = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtdeltadot = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtW = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtWdot = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtSMA = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtECC = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtINC = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtRAAN = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtAOP = wx.TextCtrl(self, -1, "", size=(300,-1))
        self.txtMA = wx.TextCtrl(self, -1, "", size=(300,-1))

        #bind text boxes
        self.txtname.Bind(wx.EVT_KILL_FOCUS, self.Changename)
        self.txtshortname.Bind(wx.EVT_KILL_FOCUS, self.Changeshortname)
        self.txtSPICE_ID.Bind(wx.EVT_KILL_FOCUS, self.ChangeSPICE_ID)
        self.txtminimum_flyby_altitude.Bind(wx.EVT_KILL_FOCUS, self.Changeminimum_flyby_altitude)
        self.txtmass.Bind(wx.EVT_KILL_FOCUS, self.Changemass)
        self.txtradius.Bind(wx.EVT_KILL_FOCUS, self.Changeradius)
        self.txtephemeris_epoch.Bind(wx.EVT_KILL_FOCUS, self.Changeephemeris_epoch)
        self.txtalpha0.Bind(wx.EVT_KILL_FOCUS, self.Changealpha0)
        self.txtalphadot.Bind(wx.EVT_KILL_FOCUS, self.Changealphadot)
        self.txtdelta0.Bind(wx.EVT_KILL_FOCUS, self.Changedelta0)
        self.txtdeltadot.Bind(wx.EVT_KILL_FOCUS, self.Changedeltadot)
        self.txtW.Bind(wx.EVT_KILL_FOCUS, self.ChangeW)
        self.txtWdot.Bind(wx.EVT_KILL_FOCUS, self.ChangeWdot)
        self.txtSMA.Bind(wx.EVT_KILL_FOCUS, self.ChangeSMA)
        self.txtECC.Bind(wx.EVT_KILL_FOCUS, self.ChangeECC)
        self.txtINC.Bind(wx.EVT_KILL_FOCUS, self.ChangeINC)
        self.txtRAAN.Bind(wx.EVT_KILL_FOCUS, self.ChangeRAAN)
        self.txtAOP.Bind(wx.EVT_KILL_FOCUS, self.ChangeAOP)
        self.txtMA.Bind(wx.EVT_KILL_FOCUS, self.ChangeMA)

        fieldsgrid = wx.FlexGridSizer(19, 2, 5, 5)
        fieldsgrid.SetFlexibleDirection(wx.VERTICAL)
        fieldsgrid.AddMany([self.lblname, self.txtname,
                            self.lblshortname, self.txtshortname,
                            self.lblSPICE_ID, self.txtSPICE_ID,
                            self.lblminimum_flyby_altitude, self.txtminimum_flyby_altitude,
                            self.lblmass, self.txtmass,
                            self.lblradius, self.txtradius,
                            self.lblephemeris_epoch, self.txtephemeris_epoch,
                            self.lblalpha0, self.txtalpha0,
                            self.lblalphadot, self.txtalphadot,
                            self.lbldelta0, self.txtdelta0,
                            self.lbldeltadot, self.txtdeltadot,
                            self.lblW, self.txtW,
                            self.lblWdot, self.txtWdot,
                            self.lblSMA, self.txtSMA,
                            self.lblECC, self.txtECC,
                            self.lblINC, self.txtINC,
                            self.lblRAAN, self.txtRAAN,
                            self.lblAOP, self.txtAOP,
                            self.lblMA, self.txtMA])

        mainbox = wx.BoxSizer(wx.HORIZONTAL)
        hspacer = wx.SizerItemSpacer
        mainbox.Add(pickerstacker)
        mainbox.AddSpacer(20)
        mainbox.Add(fieldsgrid)

        self.UpdateFields()

        self.SetSizer(mainbox)

    def UpdateBodyList(self):
        self.bodylist = []
        for body in self.universe.bodies:
            self.bodylist.append(body.name)

    def UpdateFields(self):
        if self.ActiveBody >= 0:
            self.txtname.SetValue(self.universe.bodies[self.ActiveBody].name)
            self.txtshortname.SetValue(self.universe.bodies[self.ActiveBody].shortname)
            self.txtSPICE_ID.SetValue(str(self.universe.bodies[self.ActiveBody].SPICE_ID))
            self.txtminimum_flyby_altitude.SetValue(str(self.universe.bodies[self.ActiveBody].minimum_flyby_altitude))
            self.txtmass.SetValue(str(self.universe.bodies[self.ActiveBody].mass))
            self.txtradius.SetValue(str(self.universe.bodies[self.ActiveBody].radius))
            self.txtephemeris_epoch.SetValue(str(self.universe.bodies[self.ActiveBody].ephemeris_epoch))
            self.txtalpha0.SetValue(str(self.universe.bodies[self.ActiveBody].alpha0))
            self.txtalphadot.SetValue(str(self.universe.bodies[self.ActiveBody].alphadot))
            self.txtdelta0.SetValue(str(self.universe.bodies[self.ActiveBody].delta0))
            self.txtdeltadot.SetValue(str(self.universe.bodies[self.ActiveBody].deltadot))
            self.txtW.SetValue(str(self.universe.bodies[self.ActiveBody].W))
            self.txtWdot.SetValue(str(self.universe.bodies[self.ActiveBody].Wdot))
            self.txtSMA.SetValue(str(self.universe.bodies[self.ActiveBody].SMA))
            self.txtECC.SetValue(str(self.universe.bodies[self.ActiveBody].ECC))
            self.txtINC.SetValue(str(self.universe.bodies[self.ActiveBody].INC))
            self.txtRAAN.SetValue(str(self.universe.bodies[self.ActiveBody].RAAN))
            self.txtAOP.SetValue(str(self.universe.bodies[self.ActiveBody].AOP))
            self.txtMA.SetValue(str(self.universe.bodies[self.ActiveBody].MA))

            self.BodiesPicker.SetItems(self.bodylist)
            self.BodiesPicker.SetSelection(self.ActiveBody)

    def ClickBodiesPicker(self, e):
        self.ActiveBody = self.BodiesPicker.GetSelection()
        self.UpdateFields()

    def ClickAddBody(self, e):
        self.universe.bodies.append(Universe.Body.Body())
        self.universe.bodies[-1].number = len(self.universe.bodies)
        self.UpdateBodyList()
        self.UpdateFields()

    def ClickRemoveBody(self, e):
        if len(self.bodylist) > 0:
            self.ActiveBody = self.BodiesPicker.GetSelection()
            self.universe.bodies.pop(self.ActiveBody)
            self.UpdateBodyList()
            self.ActiveBody -= 1
            self.UpdateFields()

    def ClickMoveBodyUp(self, e):
        if len(self.bodylist) > 0:
                self.ActiveBody = self.BodiesPicker.GetSelection()
                if self.ActiveBody > 0:
                    self.universe.bodies[self.ActiveBody], self.universe.bodies[self.ActiveBody - 1] = self.universe.bodies[self.ActiveBody - 1], self.universe.bodies[self.ActiveBody]
                    self.UpdateBodyList()
                    self.ActiveBody -= 1
                    self.UpdateFields()

    def ClickMoveBodyDown(self, e):
        if len(self.bodylist) > 0:
                self.ActiveBody = self.BodiesPicker.GetSelection()
                if self.ActiveBody < len(self.bodylist) - 1:
                    self.universe.bodies[self.ActiveBody], self.universe.bodies[self.ActiveBody + 1] = self.universe.bodies[self.ActiveBody + 1], self.universe.bodies[self.ActiveBody]
                    self.UpdateBodyList()
                    self.ActiveBody += 1
                    self.UpdateFields()

    def Changename(self, e):
        self.universe.bodies[self.ActiveBody].name = self.txtname.GetValue()

    def Changeshortname(self, e):
        self.universe.bodies[self.ActiveBody].shortname = self.txtshortname.GetValue()

    def ChangeSPICE_ID(self, e):
        self.universe.bodies[self.ActiveBody].SPICE_ID = eval(self.txtSPICE_ID.GetValue())

    def Changeminimum_flyby_altitude(self, e):
        r = self.universe.bodies[self.ActiveBody].radius
        self.universe.bodies[self.ActiveBody].minimum_flyby_altitude = eval(self.txtminimum_flyby_altitude.GetValue())

    def Changemass(self, e):
        self.universe.bodies[self.ActiveBody].mass = eval(self.txtmass.GetValue())

    def Changeradius(self, e):
        self.universe.bodies[self.ActiveBody].radius = eval(self.txtradius.GetValue())

    def Changeephemeris_epoch(self, e):
        self.universe.bodies[self.ActiveBody].ephemeris_epoch = eval(self.txtephemeris_epoch.GetValue())

    def Changealpha0(self, e):
        self.universe.bodies[self.ActiveBody].alpha0 = eval(self.txtalpha0.GetValue())

    def Changealphadot(self, e):
        self.universe.bodies[self.ActiveBody].alphadot = eval(self.txtalphadot.GetValue())

    def Changedelta0(self, e):
        self.universe.bodies[self.ActiveBody].delta0 = eval(self.txtdelta0.GetValue())

    def Changedeltadot(self, e):
        self.universe.bodies[self.ActiveBody].deltadot = eval(self.txtdeltadot.GetValue())

    def ChangeW(self, e):
        self.universe.bodies[self.ActiveBody].W = eval(self.txtW.GetValue())

    def ChangeWdot(self, e):
        self.universe.bodies[self.ActiveBody].Wdot = eval(self.txtWdot.GetValue())

    def ChangeSMA(self, e):
        self.universe.bodies[self.ActiveBody].SMA = eval(self.txtSMA.GetValue())

    def ChangeECC(self, e):
        self.universe.bodies[self.ActiveBody].ECC = eval(self.txtECC.GetValue())

    def ChangeINC(self, e):
        self.universe.bodies[self.ActiveBody].INC = eval(self.txtINC.GetValue())

    def ChangeRAAN(self, e):
        self.universe.bodies[self.ActiveBody].RAAN = eval(self.txtRAAN.GetValue())

    def ChangeAOP(self, e):
        self.universe.bodies[self.ActiveBody].AOP = eval(self.txtAOP.GetValue())

    def ChangeMA(self, e):
        self.universe.bodies[self.ActiveBody].MA = eval(self.txtMA.GetValue())

