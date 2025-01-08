//******************************************
// Testing Species

m = ConstructMixture();

s1 = Mixture_add(m, "H2O", 1.0)
s2 = Mixture_add(m, "H2O(L)", 1.0)
s3 = Mixture_add(m, "H2O(cr)", 1.0)
Mixture_checkFractions(m, %T);

assert_checkequal(Pointer_isSpecies(s1), %T);
assert_checkequal(Species_getName(s1), "H2O");
assert_checkequal(Species_isReactantOnly(s1), %F);
assert_checkequal(Species_isIon(s1), %F);
assert_checkequal(Species_getCharge(s1), 0);
assert_checkequal(Species_getValence(s1), 0);
assert_checkequal(Species_isCondensed(s1), %F);
assert_checkequal(Species_getCondensed(s1), 0);
assert_checkalmostequal(Species_getDHf298_15(s1), -241826, 0.001);
assert_checkalmostequal(Species_getDHf298_15(s1, "J/mol"), -241826, 0.001);
assert_checkalmostequal(Species_getDH298_15_0(s1), 9904.092, 0.001);
assert_checkalmostequal(Species_getT0(s1), 298.15, 0.001);
assert_checkalmostequal(Species_getT0(s1, "K"), 298.15, 0.001);
assert_checkalmostequal(Species_getP0(s1), 100000, 0.001);
assert_checkalmostequal(Species_getP0(s1, "Pa"), 100000, 0.001);
assert_checkalmostequal(Species_getP0(s1, "bar"), 1, 0.001);
assert_checkalmostequal(Species_getMinimumT(s1), 200, 0.001);
assert_checkalmostequal(Species_getMinimumT(s1, "K"), 200, 0.001);
assert_checkalmostequal(Species_getMaximumT(s1), 6000, 0.001);
assert_checkalmostequal(Species_getMaximumT(s1, "K"), 6000, 0.001);
assert_checkalmostequal(Species_checkT(s1, 200), 200, 0.001);
assert_checkalmostequal(Species_checkT(s1, 1000), 1000, 0.001);
assert_checkalmostequal(Species_checkT(s1, 6000), 6000, 0.001);
assert_checkerror("x = Species_checkT(s1, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_checkT(s1, 7000)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getM(s1), 18.01528, 0.001);
assert_checkalmostequal(Species_getR(s1), 0.461523329, 0.001);
assert_checkalmostequal(Species_getR(s1, "J/(mol K)"), 0.461523329, 0.001);
assert_checkalmostequal(Species_getCp(s1, 300), 33.5957719, 0.001);
assert_checkalmostequal(Species_getCp(s1, 1000), 41.2908476, 0.001);
assert_checkalmostequal(Species_getCp(s1, 6000), 62.5540249, 0.001);
assert_checkerror("x = Species_getCp(s1, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getCp(s1, 7000)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getH(s1, 300), -241762.7505646, 0.001);
assert_checkalmostequal(Species_getH(s1, 1000), -215821.66953831, 0.001);
assert_checkalmostequal(Species_getH(s1, 6000), 66056.656846788, 0.001);
assert_checkerror("x = Species_getH(s1, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getH(s1, 7000)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getS(s1, 300), 189.036042333397, 0.001);
assert_checkalmostequal(Species_getS(s1, 1000), 232.73564897165, 0.001);
assert_checkalmostequal(Species_getS(s1, 6000), 328.41169883442, 0.001);
assert_checkerror("x = Species_getS(s1, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getS(s1, 7000)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getG(s1, 300), -298473.56326465, 0.001);
assert_checkalmostequal(Species_getG(s1, 1000), -448557.31850996, 0.001);
assert_checkalmostequal(Species_getG(s1, 6000), -1904413.53615972, 0.001);
assert_checkerror("x = Species_getG(s1, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getG(s1, 7000)", "SWIG/Scilab: RuntimeError: Runtime error")

assert_checkequal(Pointer_isSpecies(s2), %T);
assert_checkequal(Species_getName(s2), "H2O(L)");
assert_checkequal(Species_isReactantOnly(s2), %F);
assert_checkequal(Species_isIon(s2), %F);
assert_checkequal(Species_getCharge(s2), 0);
assert_checkequal(Species_getValence(s2), 0);
assert_checkequal(Species_isCondensed(s2), %T);
assert_checkequal(Species_getCondensed(s2), 2);
assert_checkalmostequal(Species_getDHf298_15(s2), -285830, 0.001);
assert_checkalmostequal(Species_getDHf298_15(s2, "J/mol"), -285830, 0.001);
assert_checkalmostequal(Species_getDH298_15_0(s2), 13278, 0.001);
assert_checkalmostequal(Species_getT0(s2), 298.15, 0.001);
assert_checkalmostequal(Species_getT0(s2, "K"), 298.15, 0.001);
assert_checkalmostequal(Species_getP0(s2), 100000, 0.001);
assert_checkalmostequal(Species_getP0(s2, "Pa"), 100000, 0.001);
assert_checkalmostequal(Species_getP0(s2, "bar"), 1, 0.001);
assert_checkalmostequal(Species_getMinimumT(s2), 273.15, 0.001);
assert_checkalmostequal(Species_getMinimumT(s2, "K"), 273.15, 0.001);
assert_checkalmostequal(Species_getMaximumT(s2), 600, 0.001);
assert_checkalmostequal(Species_getMaximumT(s2, "K"), 600, 0.001);
assert_checkalmostequal(Species_checkT(s2, 273.15), 273.15, 0.001);
assert_checkalmostequal(Species_checkT(s2, 400), 400, 0.001);
assert_checkalmostequal(Species_checkT(s2, 600), 600, 0.001);
assert_checkerror("x = Species_checkT(s2, 200)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_checkT(s2, 1000)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getM(s2), 18.01528, 0.001);
assert_checkalmostequal(Species_getR(s2), 0.461523329, 0.001);
assert_checkalmostequal(Species_getR(s2, "J/(mol K)"), 0.461523329, 0.001);
assert_checkalmostequal(Species_getCp(s2, 300), 75.354608214, 0.001);
assert_checkalmostequal(Species_getCp(s2, 400), 76.799116716, 0.001);
assert_checkalmostequal(Species_getCp(s2, 600), 125.408261355, 0.001);
assert_checkerror("x = Species_getCp(s2, 200)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getCp(s2, 1000)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getH(s2, 300), -285689.37892172, 0.001);
assert_checkalmostequal(Species_getH(s2, 400), -278120.17701126, 0.001);
assert_checkalmostequal(Species_getH(s2, 600), -260445.60498218, 0.001);
assert_checkerror("x = Species_getH(s2, 200)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getH(s2, 1000)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getS(s2, 300), 70.407952124485, 0.001);
assert_checkalmostequal(Species_getS(s2, 400), 92.174015457872, 0.001);
assert_checkalmostequal(Species_getS(s2, 600), 127.51535147307, 0.001);
assert_checkerror("x = Species_getS(s2, 200)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getS(s2, 1000)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getG(s2, 300), -306811.764559282, 0.001);
assert_checkalmostequal(Species_getG(s2, 400), -314989.783194126, 0.001);
assert_checkalmostequal(Species_getG(s2, 600), -336954.815865694, 0.001);
assert_checkerror("x = Species_getG(s2, 200)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getG(s2, 1000)", "SWIG/Scilab: RuntimeError: Runtime error")

assert_checkequal(Pointer_isSpecies(s3), %T);
assert_checkequal(Species_getName(s3), "H2O(cr)");
assert_checkequal(Species_isReactantOnly(s3), %F);
assert_checkequal(Species_isIon(s3), %F);
assert_checkequal(Species_getCharge(s3), 0);
assert_checkequal(Species_getValence(s3), 0);
assert_checkequal(Species_isCondensed(s3), %T);
assert_checkequal(Species_getCondensed(s3), 1);
assert_checkalmostequal(Species_getDHf298_15(s3), -299108, 0.001);
assert_checkalmostequal(Species_getDHf298_15(s3, "J/mol"), -299108, 0.001);
assert_checkalmostequal(Species_getDH298_15_0(s3), 0, 0.001);
assert_checkalmostequal(Species_getT0(s3), 298.15, 0.001);
assert_checkalmostequal(Species_getT0(s3, "K"), 298.15, 0.001);
assert_checkalmostequal(Species_getP0(s3), 100000, 0.001);
assert_checkalmostequal(Species_getP0(s3, "Pa"), 100000, 0.001);
assert_checkalmostequal(Species_getP0(s3, "bar"), 1, 0.001);
assert_checkalmostequal(Species_getMinimumT(s3), 200, 0.001);
assert_checkalmostequal(Species_getMinimumT(s3, "K"), 200, 0.001);
assert_checkalmostequal(Species_getMaximumT(s3), 273.15, 0.001);
assert_checkalmostequal(Species_getMaximumT(s3, "K"), 273.15, 0.001);
assert_checkalmostequal(Species_checkT(s3, 200), 200, 0.001);
assert_checkalmostequal(Species_checkT(s3, 250), 250, 0.001);
assert_checkalmostequal(Species_checkT(s3, 273.15), 273.15, 0.001);
assert_checkerror("x = Species_checkT(s3, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_checkT(s3, 300)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getM(s3), 18.01528, 0.001);
assert_checkalmostequal(Species_getR(s3), 0.461523329, 0.001);
assert_checkalmostequal(Species_getR(s3, "J/(mol K)"), 0.461523329, 0.001);
assert_checkalmostequal(Species_getCp(s3, 200), 28.20416203, 0.001);
assert_checkalmostequal(Species_getCp(s3, 250), 34.87565455, 0.001);
assert_checkalmostequal(Species_getCp(s3, 273.15), 38.11271899, 0.001);
assert_checkerror("x = Species_getCp(s3, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getCp(s3, 300)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getH(s3, 200), -296145.34805529, 0.001);
assert_checkalmostequal(Species_getH(s3, 250), -294570.17426707, 0.001);
assert_checkalmostequal(Species_getH(s3, 273.15), -293725.61553537, 0.001);
assert_checkerror("x = Species_getH(s3, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getH(s3, 300)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getS(s3, 200), 31.096132371779, 0.001);
assert_checkalmostequal(Species_getS(s3, 250), 38.098266502212, 0.001);
assert_checkalmostequal(Species_getS(s3, 273.15), 41.327007662870, 0.001);
assert_checkerror("x = Species_getS(s3, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getS(s3, 300)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkalmostequal(Species_getG(s3, 200), -302364.574529647, 0.001);
assert_checkalmostequal(Species_getG(s3, 250), -304094.740892622, 0.001);
assert_checkalmostequal(Species_getG(s3, 273.15), -305014.08767849, 0.001);
assert_checkerror("x = Species_getG(s3, 100)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkerror("x = Species_getG(s3, 300)", "SWIG/Scilab: RuntimeError: Runtime error")

DeleteSpecies(s1);
DeleteSpecies(s2);
DeleteSpecies(s3);

DeleteMixture(m);

// Testing completed!
