/***************************************************
 RPA-C - tool for the thermodynamic analysis.
  
 This script demonstrates how to prepare the mixture of 
 ingredients and run the combustion problem using object Combustor.
****************************************************/

// Prepare the mixture of ingredients
mix = new Mixture();
mix.addSpecies("NH4CLO4(cr)", 0.7); 
mix.addSpecies("AL(cr)", 0.2); 
mix.addSpecies("HTPB+Curative", 0.9, "g/cm^3", 0.1); 

printf("Initial mixture:\n");
mix.print();

// Solve problem (p,H)=const using object Combustor
printf("\n\nSolve problem (p,H)=const\n");
c1 = new Combustor(mix, true, true);
c1.setP(20.7, "MPa"); 
c1.solve(true, false);
c1.getEquilibrium().print("SI");
c1.getDerivatives().print("SI");

// Solve problem (p,T)=const using object Combustor
p = 20.7;
T = 1400;
printf("\n\nSolve problem (p,T)=const at p=%4.1f MPa T=%8.3f K\n", p, T);
c2 = new Combustor(mix, true, true);
c2.setPT(p, "MPa", T, "K"); 
c2.solve(true, false);
c2.getEquilibrium().print("SI");
c2.getDerivatives().print("SI");

// Solve problem (p,H)=const using object Combustor
printf("\n\nSolve problem (p,H)=const at p=%4.1f MPa\n", p);
c3 = new Combustor(mix, true, true);
c3.setP(p, "MPa"); 
c3.solve(true, false);
c3.getEquilibrium().print("SI");
c3.getDerivatives().print("SI");
