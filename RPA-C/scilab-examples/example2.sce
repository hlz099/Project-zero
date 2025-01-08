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

ingredients = ConfigFile_getIngredients(cfg);
Ingredients_addComponent(ingredients, ConstructComponent("NH4CLO4(cr)", 0.68));
Ingredients_addComponent(ingredients, ConstructComponent("AL(cr)", 0.2));
Ingredients_addComponent(ingredients, ConstructComponent("HTPB+Curative", 0.12));

gopt = ConfigFile_getGeneralOptions(cfg);
GeneralOptions_setMultiphase(gopt, %T);
GeneralOptions_setIons(gopt, %T);

cc = ConfigFile_getCombustionConditions(cfg);
CombustionConditions_setP(cc, 20.7, "MPa");               // Will be changed in the loop below

hexc = ConfigFile_getHexConditions(cfg);
HEXConditions_setType(hexc, "none");

// Configuration is cerated
//*************************************

// Create separate copy of mixture with ingredients to get some
// propertiesof the mixture: density and equivalence ratio
// We coud get it from CombustionAnalysis, but only after the analysis run,
// which is in the loop in this example. So, to improve the performance,
// we create another copy of mixtue outside of the loop
mix = ConstructMixture();
ingredients = ConfigFile_getIngredients(cfg);
for i=0:Ingredients_getSize(ingredients)-1
    ing = Ingredients_getComponent(ingredients, i);
    s = Mixture_add(mix, Component_getName(ing), Component_getMf(ing));
end

// Get the density of the mixture in kg/m3
rho = Mixture_getRho(mix, "kg/m^3");

// Equivalence ratio
er = Mixture_getEquivalenceRatio(mix, Ingredients_getOmitAtomsER(ingredients));

//*************************************
// Run analysis

p=logspace(-1,2,30)

for i=1:size(p,'*')

    cc = ConfigFile_getCombustionConditions(cfg);
    CombustionConditions_setP(cc, p(i), "MPa"); // Update main combustion conditions

    ca = ConstructCombustionAnalysis();
    CombustionAnalysis_setPrintResults(ca, %F);
    CombustionAnalysis_run(ca, cfg);

    if CombustionAnalysis_getCombustorsListSize(ca)>0 then

        e = CombustionAnalysis_getEquilibrium(ca, 0);

        // Pressure
        _p = Equilibrium_getP(e, "MPa");

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

        mprintf("%2d: p=%f T=%f rho=%f er=%f g=%f vc=%f\n", i, _p, T, rho, er, g, vc);

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

subplot(1,3,1)
plot(p,data(1,:),'bo-','thickness',2)
xgrid(33,1,8)
title('Flame Temperature',"font_style",8,"fontsize",3)
xlabel("Pressure (MPa)","font_style",8,"fontsize",2)
ylabel("Flame Temperature (K)","font_style",8,"fontsize",2)
a=gca();a.log_flags="lnn"

subplot(1,3,2)
plot(p,data(5,:),'bo-','thickness',2)
xgrid(33,1,8)
title('Gas Yield',"font_style",8,"fontsize",3)
xlabel("Pressure (MPa)","font_style",8,"fontsize",2)
ylabel("Gas yield (mol/kg)","font_style",8,"fontsize",2)
a=gca();a.log_flags="lnn"

subplot(1,3,3)
plot(p,data(5,:).*data(1,:)/1000,'bo-','thickness',2)
xgrid(33,1,8)
title('Massic Sifx',"font_style",8,"fontsize",3)
xlabel("Pressure (MPa)","font_style",8,"fontsize",2)
ylabel("Massic Sifx (mol.K/g)","font_style",8,"fontsize",2)
a=gca();a.log_flags="lnn"

drawnow()
