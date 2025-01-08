//******************************************
// Testing Combustion Analysis

//*****************************
// Case with existing configuration file

cfg = ConstructConfigFile();
ConfigFile_read(cfg, "scilab-tests/test.cfg")

ca = ConstructCombustionAnalysis();
assert_checkequal(Pointer_isCombustionAnalysis(ca), %T);

CombustionAnalysis_run(ca, cfg);

assert_checkequal(CombustionAnalysis_isHEX(ca), %T);
assert_checkalmostequal(CombustionAnalysis_getHEX(ca), 2918998.6, 0.001);
assert_checkequal(CombustionAnalysis_isHEXInterpolated(ca), %F);


// 1 main and 1 optional conditions
assert_checkequal(CombustionAnalysis_getCombustorsListSize(ca), 2);

// Check results for main conditions

e0 = CombustionAnalysis_getEquilibrium(ca, 0);
assert_checkequal(Pointer_isEquilibrium(e0), %T);

d0 = CombustionAnalysis_getDerivatives(ca, 0);
assert_checkequal(Pointer_isDerivatives(d0), %T);

assert_checkalmostequal(Equilibrium_getP(e0), 20000000, 0.001);
assert_checkalmostequal(Equilibrium_getP(e0, "MPa"), 20, 0.001);

assert_checkalmostequal(Equilibrium_getT(e0), 1274.6012, 0.001);
assert_checkalmostequal(Equilibrium_getT(e0, "K"), 1274.6012, 0.001);

assert_checkalmostequal(Equilibrium_getH(e0), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e0, "J/mol"), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e0, "J/kg"), -5520520.096596, 0.001);

assert_checkalmostequal(Equilibrium_getU(e0), -135783.5097659, 0.001);
assert_checkalmostequal(Equilibrium_getU(e0, "J/mol"), -135783.5097659, 0.001);
assert_checkalmostequal(Equilibrium_getU(e0, "J/kg"), -5987860.8460778, 0.001);

assert_checkalmostequal(Equilibrium_getS(e0), 207.40936579, 0.001);
assert_checkalmostequal(Equilibrium_getS(e0, "J/(mol K)"), 207.40936579, 0.001);
assert_checkalmostequal(Equilibrium_getS(e0, "J/(kg K)"), 9146.4598512, 0.001);

assert_checkalmostequal(Equilibrium_getG(e0), -389550.0908053, 0.001);
assert_checkalmostequal(Equilibrium_getG(e0, "J/mol"), -389550.0908053, 0.001);
assert_checkalmostequal(Equilibrium_getG(e0, "J/kg"), -17178608.3622382, 0.001);

assert_checkalmostequal(Derivatives_getCp(d0), 41.831469, 0.001);
assert_checkalmostequal(Derivatives_getCp(d0, "J/(mol K)"), 41.831469, 0.001);

assert_checkalmostequal(Derivatives_getCv(d0), 33.516584, 0.001);
assert_checkalmostequal(Derivatives_getCv(d0, "J/(mol K)"), 33.516584, 0.001);

assert_checkalmostequal(Derivatives_getR(d0), 8.314472, 0.001);
assert_checkalmostequal(Derivatives_getR(d0, "J/(mol K)"), 8.314472, 0.001);

assert_checkalmostequal(Derivatives_getK(d0), 1.2480820, 0.001);
assert_checkalmostequal(Derivatives_getGamma(d0), 1.2480827, 0.001);

assert_checkalmostequal(Derivatives_getA(d0), 763.7274, 0.001);
assert_checkalmostequal(Derivatives_getA(d0, "m/s"), 763.7274, 0.001);

assert_checkalmostequal(Derivatives_getRho(d0), 42.7952385, 0.001);
assert_checkalmostequal(Derivatives_getRho(d0, "kg/m^3"), 42.7952385, 0.001);

assert_checkalmostequal(Derivatives_getRhoGas(d0), 42.7952385, 0.001);
assert_checkalmostequal(Derivatives_getRhoGas(d0, "kg/m^3"), 42.7952385, 0.001);

assert_checkalmostequal(Derivatives_getZ(d0), 0, 0.001);

assert_checkalmostequal(Derivatives_getP(d0), 20000000, 0.001);
assert_checkalmostequal(Derivatives_getP(d0, "MPa"), 20, 0.001);

assert_checkalmostequal(Derivatives_getMm(d0), 0.02267646, 0.001);
assert_checkalmostequal(Derivatives_getMm(d0, "kg/mol"), 0.02267646, 0.001);

assert_checkalmostequal(Derivatives_getM(d0), 22.67646, 0.001);

// Check results for optional conditions

e1 = CombustionAnalysis_getEquilibrium(ca, 1);
assert_checkequal(Pointer_isEquilibrium(e1), %T);

d1 = CombustionAnalysis_getDerivatives(ca, 1);
assert_checkequal(Pointer_isDerivatives(d1), %T);

// Check only P and T
assert_checkalmostequal(Equilibrium_getP(e0), 20000000, 0.001);
assert_checkalmostequal(Equilibrium_getP(e0, "MPa"), 20, 0.001);

assert_checkalmostequal(Equilibrium_getT(e0), 1274.6, 0.001);
assert_checkalmostequal(Equilibrium_getT(e0, "K"), 1274.6, 0.001);

DeleteCombustionAnalysis(ca);

//*****************************
// Case with created configuration

cfg = ConstructConfigFile();
ConfigFile_setName(cfg, "test")

ingredients = ConfigFile_getIngredients(cfg)
Ingredients_addComponent(ingredients, ConstructComponent("H2O2(L)", 1.0));

gopt = ConfigFile_getGeneralOptions(cfg);
GeneralOptions_setMultiphase(gopt, %T);
GeneralOptions_setIons(gopt, %T);

cc = ConfigFile_getCombustionConditions(cfg);
CombustionConditions_setP(cc, 20, "MPa");

copt = ConfigFile_setCombustionOptionalConditions(cfg);
CombustionConditions_setP(copt, 20, "MPa");
CombustionConditions_setT(copt, 1274.6, "K");

copt = ConfigFile_setCombustionOptionalConditions(cfg);
CombustionConditions_setP(copt, 20, "MPa");

hexc = ConfigFile_getHexConditions(cfg);
HEXConditions_setType(hexc, "exact method");

ca = ConstructCombustionAnalysis();
assert_checkequal(Pointer_isCombustionAnalysis(ca), %T);

CombustionAnalysis_run(ca, cfg);

e0 = CombustionAnalysis_getEquilibrium(ca, 0);
d0 = CombustionAnalysis_getDerivatives(ca, 0);

assert_checkalmostequal(Equilibrium_getP(e0), 20000000, 0.001);
assert_checkalmostequal(Equilibrium_getP(e0, "MPa"), 20, 0.001);

assert_checkalmostequal(Equilibrium_getT(e0), 1274.6012, 0.001);
assert_checkalmostequal(Equilibrium_getT(e0, "K"), 1274.6012, 0.001);

assert_checkalmostequal(Equilibrium_getH(e0), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e0, "J/mol"), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e0, "J/kg"), -5520520.096596, 0.001);

assert_checkequal(CombustionAnalysis_isHEX(ca), %T);
assert_checkalmostequal(CombustionAnalysis_getHEX(ca), 2918998.6, 0.001);
assert_checkequal(CombustionAnalysis_isHEXInterpolated(ca), %F);

// 1 main and 2 optional conditions
assert_checkequal(CombustionAnalysis_getCombustorsListSize(ca), 3);

e1 = CombustionAnalysis_getEquilibrium(ca, 1);
assert_checkequal(Pointer_isEquilibrium(e1), %T);

d1 = CombustionAnalysis_getDerivatives(ca, 1);
assert_checkequal(Pointer_isDerivatives(d1), %T);

// Check only P and T
assert_checkalmostequal(Equilibrium_getP(e0), 20000000, 0.001);
assert_checkalmostequal(Equilibrium_getP(e0, "MPa"), 20, 0.001);

assert_checkalmostequal(Equilibrium_getT(e0), 1274.6, 0.001);
assert_checkalmostequal(Equilibrium_getT(e0, "K"), 1274.6, 0.001);

DeleteCombustionAnalysis(ca);

// Testing completed!
