/***************************************************
 RPA-C - tool for the thermodynamic analysis.
  
 This script demonstrates how to obtain the mixture of 
 reaction products.
****************************************************/

c = new ConfigFile("examples/HMX.cfg");
c.read();

ca = new CombustionAnalysis();
ca.run(c);

// Obtain object "Reaction" (aka "Equilibrium")
r = ca.getCombustor(0).getEquilibrium();

// Obtain mixture of reaction products
products = r.getResultingMixture();

printf("%15s %9s %9s %4s\n", "Name", "Mass Frac", "Mole Frac", "Cond");

sum1 = 0;
sum2 = 0; 

for (i=0; i<products.size(); ++i) {
    // Reaction product
    s = products.getSpecies(i);

    massFraction = products.getFraction(i, "mass"); 
    moleFraction = products.getFraction(i, "mole");; 

    sum1 += massFraction;
    sum2 += moleFraction;

    // We are printing out mass fraction in  format "%9.7f",
    // so skip all products with massFraction<1e-7
    if (massFraction<1e-7) {
        continue;
    }

    printf(
        "%15s %9.7f %9.7f %4d\n",
        s.getName(),
        massFraction,
        moleFraction,
        s.getCondensed()
    );

}

printf(
    "%15s %9.7f %9.7f\n",
    "Summ:",
    sum1,
    sum2
);
