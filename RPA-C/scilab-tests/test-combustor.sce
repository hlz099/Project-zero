//******************************************
// Testing Combustor

m = ConstructMixture();
s = Mixture_add(m, "H2O2(L)", 1.0)

//*****************************
// Case P (implicit PH)

c = ConstructCombustor(m);
assert_checkequal(Pointer_isCombustor(c), %T);

Combustor_setP(c, 20, "MPa");
Combustor_solve(c);

e = Combustor_getEquilibrium(c);
assert_checkequal(Pointer_isEquilibrium(e), %T);

d = Combustor_getDerivatives(c);
assert_checkequal(Pointer_isDerivatives(d), %T);

assert_checkalmostequal(Equilibrium_getP(e), 20000000, 0.001);
assert_checkalmostequal(Equilibrium_getP(e, "MPa"), 20, 0.001);

assert_checkalmostequal(Equilibrium_getT(e), 1274.6012, 0.001);
assert_checkalmostequal(Equilibrium_getT(e, "K"), 1274.6012, 0.001);

assert_checkalmostequal(Equilibrium_getH(e), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e, "J/mol"), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e, "J/kg"), -5520520.096596, 0.001);

assert_checkalmostequal(Equilibrium_getU(e), -135783.5097659, 0.001);
assert_checkalmostequal(Equilibrium_getU(e, "J/mol"), -135783.5097659, 0.001);
assert_checkalmostequal(Equilibrium_getU(e, "J/kg"), -5987860.8460778, 0.001);

assert_checkalmostequal(Equilibrium_getS(e), 207.40936579, 0.001);
assert_checkalmostequal(Equilibrium_getS(e, "J/(mol K)"), 207.40936579, 0.001);
assert_checkalmostequal(Equilibrium_getS(e, "J/(kg K)"), 9146.4598512, 0.001);

assert_checkalmostequal(Equilibrium_getG(e), -389550.0908053, 0.001);
assert_checkalmostequal(Equilibrium_getG(e, "J/mol"), -389550.0908053, 0.001);
assert_checkalmostequal(Equilibrium_getG(e, "J/kg"), -17178608.3622382, 0.001);

assert_checkalmostequal(Derivatives_getCp(d), 41.831469, 0.001);
assert_checkalmostequal(Derivatives_getCp(d, "J/(mol K)"), 41.831469, 0.001);

assert_checkalmostequal(Derivatives_getCv(d), 33.516584, 0.001);
assert_checkalmostequal(Derivatives_getCv(d, "J/(mol K)"), 33.516584, 0.001);

assert_checkalmostequal(Derivatives_getR(d), 8.314472, 0.001);
assert_checkalmostequal(Derivatives_getR(d, "J/(mol K)"), 8.314472, 0.001);

assert_checkalmostequal(Derivatives_getK(d), 1.2480820, 0.001);
assert_checkalmostequal(Derivatives_getGamma(d), 1.2480827, 0.001);

assert_checkalmostequal(Derivatives_getA(d), 763.7274, 0.001);
assert_checkalmostequal(Derivatives_getA(d, "m/s"), 763.7274, 0.001);

assert_checkalmostequal(Derivatives_getRho(d), 42.7952385, 0.001);
assert_checkalmostequal(Derivatives_getRho(d, "kg/m^3"), 42.7952385, 0.001);

assert_checkalmostequal(Derivatives_getRhoGas(d), 42.7952385, 0.001);
assert_checkalmostequal(Derivatives_getRhoGas(d, "kg/m^3"), 42.7952385, 0.001);

assert_checkalmostequal(Derivatives_getZ(d), 0, 0.001);

assert_checkalmostequal(Derivatives_getP(d), 20000000, 0.001);
assert_checkalmostequal(Derivatives_getP(d, "MPa"), 20, 0.001);

assert_checkalmostequal(Derivatives_getMm(d), 0.02267646, 0.001);
assert_checkalmostequal(Derivatives_getMm(d, "kg/mol"), 0.02267646, 0.001);

assert_checkalmostequal(Derivatives_getM(d), 22.67646, 0.001);

DeleteCombustor(c);

//*****************************
// Case PH (explicit)

c = ConstructCombustor(m);
assert_checkequal(Pointer_isCombustor(c), %T);

Combustor_setPH(c, 20, "MPa", Mixture_getH(m, "J/mol"), "J/mol");
Combustor_solve(c);

e = Combustor_getEquilibrium(c);
assert_checkequal(Pointer_isEquilibrium(e), %T);

d = Combustor_getDerivatives(c);
assert_checkequal(Pointer_isDerivatives(d), %T);

assert_checkalmostequal(Equilibrium_getP(e), 20000000, 0.001);
assert_checkalmostequal(Equilibrium_getP(e, "MPa"), 20, 0.001);

assert_checkalmostequal(Equilibrium_getT(e), 1274.6012, 0.001);
assert_checkalmostequal(Equilibrium_getT(e, "K"), 1274.6012, 0.001);

assert_checkalmostequal(Equilibrium_getH(e), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e, "J/mol"), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e, "J/kg"), -5520520.096596, 0.001);

assert_checkalmostequal(Equilibrium_getU(e), -135783.5097659, 0.001);
assert_checkalmostequal(Equilibrium_getU(e, "J/mol"), -135783.5097659, 0.001);
assert_checkalmostequal(Equilibrium_getU(e, "J/kg"), -5987860.8460778, 0.001);

assert_checkalmostequal(Equilibrium_getS(e), 207.40936579, 0.001);
assert_checkalmostequal(Equilibrium_getS(e, "J/(mol K)"), 207.40936579, 0.001);
assert_checkalmostequal(Equilibrium_getS(e, "J/(kg K)"), 9146.4598512, 0.001);

assert_checkalmostequal(Equilibrium_getG(e), -389550.0908053, 0.001);
assert_checkalmostequal(Equilibrium_getG(e, "J/mol"), -389550.0908053, 0.001);
assert_checkalmostequal(Equilibrium_getG(e, "J/kg"), -17178608.3622382, 0.001);

assert_checkalmostequal(Derivatives_getCp(d), 41.831469, 0.001);
assert_checkalmostequal(Derivatives_getCp(d, "J/(mol K)"), 41.831469, 0.001);

assert_checkalmostequal(Derivatives_getCv(d), 33.516584, 0.001);
assert_checkalmostequal(Derivatives_getCv(d, "J/(mol K)"), 33.516584, 0.001);

assert_checkalmostequal(Derivatives_getR(d), 8.314472, 0.001);
assert_checkalmostequal(Derivatives_getR(d, "J/(mol K)"), 8.314472, 0.001);

assert_checkalmostequal(Derivatives_getK(d), 1.2480820, 0.001);
assert_checkalmostequal(Derivatives_getGamma(d), 1.2480827, 0.001);

assert_checkalmostequal(Derivatives_getA(d), 763.7274, 0.001);
assert_checkalmostequal(Derivatives_getA(d, "m/s"), 763.7274, 0.001);

assert_checkalmostequal(Derivatives_getRho(d), 42.7952385, 0.001);
assert_checkalmostequal(Derivatives_getRho(d, "kg/m^3"), 42.7952385, 0.001);

assert_checkalmostequal(Derivatives_getRhoGas(d), 42.7952385, 0.001);
assert_checkalmostequal(Derivatives_getRhoGas(d, "kg/m^3"), 42.7952385, 0.001);

assert_checkalmostequal(Derivatives_getZ(d), 0, 0.001);

assert_checkalmostequal(Derivatives_getP(d), 20000000, 0.001);
assert_checkalmostequal(Derivatives_getP(d, "MPa"), 20, 0.001);

assert_checkalmostequal(Derivatives_getMm(d), 0.02267646, 0.001);
assert_checkalmostequal(Derivatives_getMm(d, "kg/mol"), 0.02267646, 0.001);

assert_checkalmostequal(Derivatives_getM(d), 22.67646, 0.001);

DeleteCombustor(c);

//*****************************
// Case PT

c = ConstructCombustor(m);
assert_checkequal(Pointer_isCombustor(c), %T);

Combustor_setPT(c, 20, "MPa", 1274.6012, "K");
Combustor_solve(c);

e = Combustor_getEquilibrium(c);
assert_checkequal(Pointer_isEquilibrium(e), %T);

d = Combustor_getDerivatives(c);
assert_checkequal(Pointer_isDerivatives(d), %T);

assert_checkalmostequal(Equilibrium_getP(e), 20000000, 0.001);
assert_checkalmostequal(Equilibrium_getP(e, "MPa"), 20, 0.001);

assert_checkalmostequal(Equilibrium_getT(e), 1274.6012, 0.001);
assert_checkalmostequal(Equilibrium_getT(e, "K"), 1274.6012, 0.001);

assert_checkalmostequal(Equilibrium_getH(e), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e, "J/mol"), -125185.87417, 0.001);
assert_checkalmostequal(Equilibrium_getH(e, "J/kg"), -5520520.096596, 0.001);

assert_checkalmostequal(Equilibrium_getU(e), -135783.5097659, 0.001);
assert_checkalmostequal(Equilibrium_getU(e, "J/mol"), -135783.5097659, 0.001);
assert_checkalmostequal(Equilibrium_getU(e, "J/kg"), -5987860.8460778, 0.001);

assert_checkalmostequal(Equilibrium_getS(e), 207.40936579, 0.001);
assert_checkalmostequal(Equilibrium_getS(e, "J/(mol K)"), 207.40936579, 0.001);
assert_checkalmostequal(Equilibrium_getS(e, "J/(kg K)"), 9146.4598512, 0.001);

assert_checkalmostequal(Equilibrium_getG(e), -389550.0908053, 0.001);
assert_checkalmostequal(Equilibrium_getG(e, "J/mol"), -389550.0908053, 0.001);
assert_checkalmostequal(Equilibrium_getG(e, "J/kg"), -17178608.3622382, 0.001);

assert_checkalmostequal(Derivatives_getCp(d), 41.831469, 0.001);
assert_checkalmostequal(Derivatives_getCp(d, "J/(mol K)"), 41.831469, 0.001);

assert_checkalmostequal(Derivatives_getCv(d), 33.516584, 0.001);
assert_checkalmostequal(Derivatives_getCv(d, "J/(mol K)"), 33.516584, 0.001);

assert_checkalmostequal(Derivatives_getR(d), 8.314472, 0.001);
assert_checkalmostequal(Derivatives_getR(d, "J/(mol K)"), 8.314472, 0.001);

assert_checkalmostequal(Derivatives_getK(d), 1.2480820, 0.001);
assert_checkalmostequal(Derivatives_getGamma(d), 1.2480827, 0.001);

assert_checkalmostequal(Derivatives_getA(d), 763.7274, 0.001);
assert_checkalmostequal(Derivatives_getA(d, "m/s"), 763.7274, 0.001);

assert_checkalmostequal(Derivatives_getRho(d), 42.7952385, 0.001);
assert_checkalmostequal(Derivatives_getRho(d, "kg/m^3"), 42.7952385, 0.001);

assert_checkalmostequal(Derivatives_getRhoGas(d), 42.7952385, 0.001);
assert_checkalmostequal(Derivatives_getRhoGas(d, "kg/m^3"), 42.7952385, 0.001);

assert_checkalmostequal(Derivatives_getZ(d), 0, 0.001);

assert_checkalmostequal(Derivatives_getP(d), 20000000, 0.001);
assert_checkalmostequal(Derivatives_getP(d, "MPa"), 20, 0.001);

assert_checkalmostequal(Derivatives_getMm(d), 0.02267646, 0.001);
assert_checkalmostequal(Derivatives_getMm(d, "kg/mol"), 0.02267646, 0.001);

assert_checkalmostequal(Derivatives_getM(d), 22.67646, 0.001);

DeleteCombustor(c);


// Testing completed!


