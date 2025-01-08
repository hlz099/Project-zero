//******************************************
// Testing Mixture, Exploded Formula

m = ConstructMixture();

assert_checkequal(Pointer_isMixture(m), %T);
assert_checkequal(Pointer_isSpecies(m), %F);
assert_checkequal(Mixture_size(m), 0);

s = Mixture_add(m, "H2O(L)", 1.0)
assert_checkequal(Pointer_isSpecies(s), %T);
assert_checkequal(Species_getName(s), "H2O(L)");
DeleteSpecies(s);

assert_checkequal(Mixture_size(m), 1);
s2 = Mixture_getSpecies(m, 0);
assert_checkequal(Pointer_isSpecies(s2), %T);
assert_checkequal(Species_getName(s2), "H2O(L)");
assert_checkerror("x = Mixture_getSpecies(m, 1)", "SWIG/Scilab: RuntimeError: Runtime error")
DeleteSpecies(s2);

assert_checkalmostequal(Mixture_getValence(m), 0, 0.001);
assert_checkalmostequal(Mixture_getEquivalenceRatio(m), 1, 0.001);
assert_checkalmostequal(Mixture_getOxygenBalance(m), 0, 0.001);

assert_checkalmostequal(Mixture_getM(m), 18.01534, 0.001);
assert_checkalmostequal(Mixture_getH(m), -285828.75759, 0.001);
assert_checkalmostequal(Mixture_getH(m, "J/mol"), -285828.75759, 0.001);
assert_checkalmostequal(Mixture_getH(m, "J/kg"), -15865845.38165, 0.001);
assert_checkalmostequal(Mixture_getU(m), -285828.78154, 0.001);
assert_checkalmostequal(Mixture_getU(m, "J/mol"), -285828.78154, 0.001);
assert_checkalmostequal(Mixture_getU(m, "J/kg"), -15865846.71072, 0.001);

assert_checkalmostequal(Mixture_getRho(m), 996.93325, 0.001);
assert_checkalmostequal(Mixture_getRho(m, "kg/m^3"), 996.93325, 0.001);

f = Mixture_getFormula(m);
disp(f)
assert_checkequal(Pointer_isFormula(f), %T);
assert_checkequal(Formula_size(f), 2);
assert_checkequal(Formula_getElement(f, 0), "H");
assert_checkalmostequal(Formula_getNumber(f, 0), 66.66667, 0.001); // Default is formula by %
assert_checkequal(Formula_getElement(f, 1), "O");
assert_checkalmostequal(Formula_getNumber(f, 1), 33.33333, 0.001); // Default is formula by %
//assert_checkerror("x = Formula_getElement(f, 2)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Formula_getNumber(f, 2)", "SWIG/Scilab: RuntimeError: Runtime error")
DeleteFormula(f);

DeleteMixture(m);

m = ConstructMixture();
s = Mixture_add(m, "Cu2O", 1.0)
assert_checkalmostequal(Mixture_getEquivalenceRatio(m), 0.5, 0.001);
assert_checkalmostequal(Mixture_getOxygenBalance(m), -0.1118124, 0.001);
assert_checkalmostequal(Mixture_getEquivalenceRatio(m, "O"), 0, 0.001);
assert_checkalmostequal(Mixture_getOxygenBalance(m, "Cu"), -0.1118124, 0.001);
DeleteMixture(m);

m = ConstructMixture();
s1 = Mixture_add(m, "Cu2O", 0.2)
s2 = Mixture_add(m, "H2O(L)", 0.3)
assert_checkequal(Mixture_size(m), 2);

assert_checkerror("x = Mixture_getFraction(m, 0)", "SWIG/Scilab: RuntimeError: Runtime error");
assert_checkerror("x = Mixture_getFraction(m, 1)", "SWIG/Scilab: RuntimeError: Runtime error");
assert_checkerror("Mixture_checkFractions(m)", "SWIG/Scilab: RuntimeError: Runtime error");
Mixture_checkFractions(m, %T);
assert_checkequal(Mixture_getFraction(m, 0), 0.4);
assert_checkequal(Mixture_getFraction(m, 1), 0.6);

Mixture_setFraction(m, 0, 0.7);
Mixture_setFraction(m, 1, 0.7);
assert_checkerror("x = Mixture_getFraction(m, 0)", "SWIG/Scilab: RuntimeError: Runtime error");
assert_checkerror("x = Mixture_getFraction(m, 1)", "SWIG/Scilab: RuntimeError: Runtime error");
assert_checkerror("Mixture_checkFractions(m)", "SWIG/Scilab: RuntimeError: Runtime error");
Mixture_checkFractions(m, %T);
assert_checkequal(Mixture_getFraction(m, 0), 0.5);
assert_checkequal(Mixture_getFraction(m, 1), 0.5);

DeleteMixture(m);
