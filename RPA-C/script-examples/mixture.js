/***************************************************
 RPA-C - tool for the thermodynamic analysis.
  
 This script demonstrates how to prepare the mixture of ingredients
 and print-out its properties using two different ways:
 - using function print(),
 - using other functions to obtain required properties in print out them individually in required format.
****************************************************/

// Prepare the mixture of ingredients.
mix = new Mixture();
mix.addSpecies("NH4CLO4(cr)", 0.7); 
mix.addSpecies("AL(cr)", 0.2); 
mix.addSpecies("HTPB+Curative", 0.9, "g/cm^3", 0.1); 

printf("Initial mixture (built-in printout)\n");
printf("**********************************\n");
mix.print();

printf("\nInitial mixture (script printout)\n");
printf("**********************************\n");

printf("-------------------------------------------------------------\n");
printf("%20s\t %10s\t %10s\n", "Propellant Mixture", "Mass", "Mole");
printf("%20s\t %10s\t %10s\n", "", "fractions", "fractions");
printf("-------------------------------------------------------------\n");
sum1 = 0;
sum2 = 0;
for (i=0; i<mix.size(); ++i) {
    s = mix.getSpecies(i);

    massFraction = mix.getFraction(i, "mass"); 
    moleFraction = mix.getFraction(i, "mole");; 
    sum1 += massFraction;
    sum2 += moleFraction;

    printf("%20s\t %10.7f\t %10.7f \n", s.getName(), massFraction, moleFraction);
}
printf("-------------------------------------------------------------\n");
printf("%20s\t %10.7f\t %10.7f\n", "Total:", sum1, sum2);
printf("-------------------------------------------------------------\n");

printf("Exploded chemical formula:\n");

printf("%20s\t ", "based on 1 mole:");
f = mix.getFormula("m");
for (var i=0; i<f.length; i++) {
    printf("(%s)%8.6f ", f[i].name, f[i].n);
}
printf("\n");

printf("%20s\t ", "based on 100 g:");
f = mix.getFormula("g");
for (var i=0; i<f.length; i++) {
    printf("(%s)%8.6f ", f[i].name, f[i].n);
}
printf("\n\n");

omit = ["CU"];
printf("Equivalence ratio: %8.6f (omitted: %s)\n", mix.getEquivalenceRatio(omit), omit);

omit = [];
printf("Equivalence ratio: %8.6f (omitted: %s)\n", mix.getEquivalenceRatio(omit), omit);

printf("\n");
