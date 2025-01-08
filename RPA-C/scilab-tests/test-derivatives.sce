//******************************************
// Testing Derivatives

m = ConstructMixture();
s = Mixture_add(m, "H2O2(L)", 1.0)

e = ConstructEquilibrium(m)
assert_checkequal(Pointer_isEquilibrium(e), %T);

Equilibrium_setP(e, 20, "MPa")
d = Equilibrium_solve(e)
assert_checkequal(Pointer_isDerivatives(d), %T);

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

DeleteEquilibrium(e);

DeleteMixture(m);

// Testing completed!
