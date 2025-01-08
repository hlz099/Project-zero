/***************************************************
 RPA-C - tool for the thermodynamic analysis.
  
 This script demonstrates how to prepare the mixture of 
 ingredients and run nested combustion problems using 
 object Combustor.
****************************************************/

// Prepare the mixture of ingredients
mix = new Mixture();
mix.addSpecies("NH4CLO4(cr)", 0.7); 
mix.addSpecies("AL(cr)", 0.2); 
mix.addSpecies("HTPB+Curative", 0.9, "g/cm^3", 0.1); 

printf("Initial mixture:\n");
mix.print();

// Nested analysis

// Array of pressure values in MPa
p = [10, 15, 20];

// Array of temperature values in K
// "-1" means the temperature won't be assigned 
// (see the code below)
T = [-1, 1000, 1400, 1800]; 

for (var i=0; i<p.length; i++) {
    printf("Pressure p=%10.5f MPa\n", p[i]);

    for (var j=0; j<T.length; j++) {
        c = new Combustor(mix, true, true);

        if (T[j]>0) {
            printf("Temperature T=%10.5f K\n", T[j]);
            c.setPT(p[i], "MPa", T[j], "K"); 
        } else {
            c.setP(p[i], "MPa"); 
        }

        c.solve(true, false);

        c.getEquilibrium().print("SI");
        c.getDerivatives().print("SI");

        printf("\n*************************************************************\n");
    }

}

