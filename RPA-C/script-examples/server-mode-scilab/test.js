/* Prepare the mixture of ingredients */
mix = new Mixture();
mix.addSpecies("NH4CLO4(cr)", 0.7);
mix.addSpecies("AL(cr)", 0.2);
mix.addSpecies("HTPB+Curative", 0.9, "g/cm^3", 0.1);

/* Solve problem (p,H)=const using object Combustor */
c = new Combustor(mix, true, true);
c.setP(20.7, "MPa");
c.solve(true, false);
e = c.getEquilibrium();
d = c.getDerivatives();
