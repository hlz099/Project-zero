exec loader.sce
RPAInit()

// clear console
clc
// clear memory
clear
// close all plots
xdel(winsid())

tic()

//*************************************
// Create configuration in memory

cfg = ConstructConfigFile();
ConfigFile_setName(cfg, "Test")

// We will change mass fractions for these two ingredients in the loop
// so assign them to separate variables to access later easier
c_al = ConstructComponent("AL(cr)", 0.20);
c_htpb = ConstructComponent("HTPB+Curative", 0.12);
c_total_mf = Component_getMf(c_al) + Component_getMf(c_htpb);

ingredients = ConfigFile_getIngredients(cfg);
Ingredients_addComponent(ingredients, ConstructComponent("NH4CLO4(cr)", 0.68));
Ingredients_addComponent(ingredients, c_al);
Ingredients_addComponent(ingredients, c_htpb);

gopt = ConfigFile_getGeneralOptions(cfg);
GeneralOptions_setMultiphase(gopt, %T);
GeneralOptions_setIons(gopt, %T);

cc = ConfigFile_getCombustionConditions(cfg);
CombustionConditions_setP(cc, 20.7, "MPa");               // Will be changed in the loop below

hexc = ConfigFile_getHexConditions(cfg);
HEXConditions_setType(hexc, "none");

//*************************************
// Run analysis

y1=.4
y2=.6
x1=1
x2=30

for i=x1:x2
    txg(i)=((y1-y2)/(x1-x2))*i+(-(x2*y1-x1*y2)/(x1-x2))
    txb(i)=c_total_mf-txg(i)

    // Re-assign the mass fractions
    Component_setMf(c_al, txg(i));
    Component_setMf(c_htpb, txb(i));

    ca = ConstructCombustionAnalysis();
    CombustionAnalysis_setPrintResults(ca, %F);
    CombustionAnalysis_run(ca, cfg);

    if CombustionAnalysis_getCombustorsListSize(ca)>0 then

        // Get current initial mixture (ingredients)
        mix = CombustionAnalysis_getMixture(ca);

        // Get the density of the mixture in kg/m3
        rho = Mixture_getRho(mix, "kg/m^3");

        // Equivalence ratio
        er = Mixture_getEquivalenceRatio(mix, Ingredients_getOmitAtomsER(ingredients));

        e = CombustionAnalysis_getEquilibrium(ca, 0);

        // Pressure
        p = Equilibrium_getP(e, "MPa");

        // Flame Temperature in K
        T = Equilibrium_getT(e, "K");

        // Gas Yield (mol/kg)
        products = Equilibrium_getResultingMixture(e);
        v = 1000 / Mixture_getM(products);	// Mole number in 100 gm of whole mixture
        v_c = 0;				// Mole number in 100 gm of condenced mixture
        for j=0:Mixture_size(products)-1
            s = Mixture_getSpecies(products, j);
            if Species_isCondensed(s) then
                v_c = v_c + (Mixture_getFraction(products, j, "mole") * v);
            end
        end
        g = v - v_c;

        // Condensed fraction
        vc = 0;
        d = CombustionAnalysis_getDerivatives(ca, 0);
        vc = Derivatives_getZ(d);

        mprintf("%2d: Mf=%f+%f=%f p=%f T=%f rho=%f er=%f g=%f vc=%f\n", i, txg(i), txb(i), c_total_mf, p, T, rho, er, g, vc);

        data(:,i) = [T, rho, er, g, vc];

    end
    DeleteCombustionAnalysis(ca);
end

d=toc()
disp(d)

//*************************************
// Plot diagrams

xdel()
f=figure("background",-2,"figure_position", [0 0],"figure_size",[1800 1000]);
drawlater()

subplot(2,3,1)
plot(txg',data(1,:),'bo-','thickness',2)
xgrid(33,1,8)
title('Flame Temperature',"font_style",8,"fontsize",3)
xlabel("Guni rate","font_style",8,"fontsize",2)
ylabel("Flame Temperature (K)","font_style",8,"fontsize",2)

subplot(2,3,2)
plot(txg',data(4,:),'bo-','thickness',2)
xgrid(33,1,8)
title('Gas Yield',"font_style",8,"fontsize",3)
xlabel("Guni rate","font_style",8,"fontsize",2)
ylabel("Gas yield (mol/kg)","font_style",8,"fontsize",2)

subplot(2,3,3)
plot(txg',data(4,:).*data(1,:)/1000,'bo-','thickness',2)
xgrid(33,1,8)
title('Massic Sifx',"font_style",8,"fontsize",3)
xlabel("Guni rate","font_style",8,"fontsize",2)
ylabel("Massic Sifx (mol.K/g)","font_style",8,"fontsize",2)

subplot(2,3,4)
plot(txg',data(2,:),'bo-','thickness',2)
xgrid(33,1,8)
title('Density',"font_style",8,"fontsize",3)
xlabel("Guni rate","font_style",8,"fontsize",2)
ylabel("Density (g/m3)","font_style",8,"fontsize",2)

subplot(2,3,5)
plot(txg',data(3,:),'bo-','thickness',2)
xgrid(33,1,8)
title('Equivalent Ratio',"font_style",8,"fontsize",3)
xlabel("Guni rate","font_style",8,"fontsize",2)
ylabel("Equivalent ratio","font_style",8,"fontsize",2)

subplot(2,3,6)
plot(txg',data(5,:),'bo-','thickness',2)
xgrid(33,1,8)
title('Condensed phase',"font_style",8,"fontsize",3)
xlabel("Guni rate","font_style",8,"fontsize",2)
ylabel("Condensed Phase (%)","font_style",8,"fontsize",2)

drawnow()
