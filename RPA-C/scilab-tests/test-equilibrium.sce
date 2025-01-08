//******************************************
// Testing Equilibrium

m = ConstructMixture();
s = Mixture_add(m, "H2O2(L)", 1.0)

//*****************************
// Case P (implicit PH)

e = ConstructEquilibrium(m)
assert_checkequal(Pointer_isEquilibrium(e), %T);

Equilibrium_setP(e, 20, "MPa")
d = Equilibrium_solve(e)
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

assert_checkequal(Equilibrium_hasCondensedPhase(e), %F);

rm = Equilibrium_getResultingMixture(e);
assert_checkequal(Pointer_isMixture(rm), %T);

expected(1).name = "H"
expected(1).mass = 1.540D-13
expected(1).mole = 3.464D-12
expected(1).ion = %F
expected(1).condensed = %F

expected(2).name = "H+"
expected(2).mass = 3.223D-39
expected(2).mole = 7.255D-38
expected(2).ion = %T
expected(2).condensed = %F

expected(3).name = "H-"
expected(3).mass = 3.011D-39
expected(3).mole = 6.771D-38
expected(3).ion = %T
expected(3).condensed = %F

expected(4).name = "H2"
expected(4).mass = 3.972D-10
expected(4).mole = 4.468D-09
expected(4).ion = %F
expected(4).condensed = %F

expected(5).name = "H2+"
expected(5).mass = 4.422D-42
expected(5).mole = 4.975D-41
expected(5).ion = %T
expected(5).condensed = %F

expected(6).name = "H2-"
expected(6).mass = 1.215D-47
expected(6).mole = 1.367D-46
expected(6).ion = %T
expected(6).condensed = %F

expected(7).name = "H2O"
expected(7).mass = 0.5296292
expected(7).mole = 0.6666615
expected(7).ion = %F
expected(7).condensed = %F

expected(8).name = "H2O+"
expected(8).mass = 3.921D-22
expected(8).mole = 4.936D-22
expected(8).ion = %T
expected(8).condensed = %F

expected(9).name = "H2O2"
expected(9).mass = 0.0000004
expected(9).mole = 0.0000002
expected(9).ion = %F
expected(9).condensed = %F

expected(10).name = "H3O+"
expected(10).mass = 3.025D-13
expected(10).mole = 3.606D-13
expected(10).ion = %T
expected(10).condensed = %F

expected(11).name = "HO2"
expected(11).mass = 0.0000006
expected(11).mole = 0.0000004
expected(11).ion = %F
expected(11).condensed = %F

expected(12).name = "HO2-"
expected(12).mass = 4.242D-31
expected(12).mole = 2.914D-31
expected(12).ion = %T
expected(12).condensed = %F

expected(13).name = "O"
expected(13).mass = 3.193D-09
expected(13).mole = 4.526D-09
expected(13).ion = %F
expected(13).condensed = %F

expected(14).name = "O+"
expected(14).mass = 5.407D-35
expected(14).mole = 7.663D-35
expected(14).ion = %T
expected(14).condensed = %F

expected(15).name = "O-"
expected(15).mass = 5.292D-32
expected(15).mole = 7.500D-32
expected(15).ion = %T
expected(15).condensed = %F

expected(16).name = "O2"
expected(16).mass = 0.4703643
expected(16).mole = 0.3333305
expected(16).ion = %F
expected(16).condensed = %F

expected(17).name = "O2+"
expected(17).mass = 2.025D-20
expected(17).mole = 1.435D-20
expected(17).ion = %T
expected(17).condensed = %F

expected(18).name = "O2-"
expected(18).mass = 1.963D-27
expected(18).mole = 1.391D-27
expected(18).ion = %T
expected(18).condensed = %F

expected(19).name = "O3"
expected(19).mass = 2.353D-09
expected(19).mole = 1.111D-09
expected(19).ion = %F
expected(19).condensed = %F

expected(20).name = "OH"
expected(20).mass = 0.0000055
expected(20).mole = 0.0000073
expected(20).ion = %F
expected(20).condensed = %F

expected(21).name = "OH+"
expected(21).mass = 4.105D-29
expected(21).mole = 5.474D-29
expected(21).ion = %T
expected(21).condensed = %F

expected(22).name = "OH-"
expected(22).mass = 9.460D-28
expected(22).mole = 1.261D-27
expected(22).ion = %T
expected(22).condensed = %F

expected(23).name = "e-"
expected(23).mass = 1.904D-34
expected(23).mole = 7.870D-30
expected(23).ion = %T
expected(23).condensed = %F

for i=0:Mixture_size(rm)-1
    s = Mixture_getSpecies(rm, i);
    assert_checkequal(Pointer_isSpecies(s), %T);

    ex = expected(i+1);
    assert_checkequal(Species_getName(s), ex.name);
    assert_checkalmostequal(Mixture_getFraction(rm, i, "mass"), ex.mass, 0.2);
    assert_checkalmostequal(Mixture_getFraction(rm, i, "mole"), ex.mole, 0.2);
    assert_checkequal(Species_isIon(s), ex.ion);
    assert_checkequal(Species_isCondensed(s), ex.condensed);
end

DeleteEquilibrium(e);

//*****************************
// Case PH (explicit)

e = ConstructEquilibrium(m)
assert_checkequal(Pointer_isEquilibrium(e), %T);

Equilibrium_setPH(e, 20, "MPa", Mixture_getH(m, "J/mol"), "J/mol")
d = Equilibrium_solve(e)
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

assert_checkequal(Equilibrium_hasCondensedPhase(e), %F);

DeleteEquilibrium(e);

//*****************************
// Case PT

e = ConstructEquilibrium(m)
assert_checkequal(Pointer_isEquilibrium(e), %T);

Equilibrium_setPT(e, 20, "MPa", 1274.6012, "K")
d = Equilibrium_solve(e)
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

assert_checkequal(Equilibrium_hasCondensedPhase(e), %F);

DeleteEquilibrium(e);

//*****************************

DeleteMixture(m);

// Testing completed!



