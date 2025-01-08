/***************************************************
 RPA-C - tool for the thermodynamic analysis.
  
 This script demonstrates how to write results in required
 format into the file "log.txt".
****************************************************/

// Open the  file "log.txt" in the mode "w" ("write")
var f = new File("log.txt", "w"); 

// Define variable with the name of configuration file
configName = "examples/HMX.cfg";

// Open configuration file
c = new ConfigFile(configName);
c.read();
f.printf("# Configuration file: %s\n\n", configName);

// Prepare and run combustion analysis
ca = new CombustionAnalysis();
ca.run(c);

if (ca.getCombustorsListSize()>0) {
    combustor = ca.getCombustor(0);

    r = combustor.getEquilibrium();
    products = r.getResultingMixture();

    unit = "MPa";
    f.printf("p   = %10.5f %s\n", combustor.getEquilibrium().getP(unit), unit);

    unit = "K";
    f.printf("T   = %10.5f %s\n", combustor.getEquilibrium().getT(unit), unit);

    unit = "kJ/kg";
    f.printf("HEX = %10.5f %s\n", ca.getHEX(unit), unit);
   
    f.printf("\n# %13s %9s %9s %4s\n", "Name", "Mass Frac", "Mole Frac", "Cond");

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

        f.printf("%15s %9.7f %9.7f %4d\n",
            s.getName(),
            massFraction,
            moleFraction,
            s.getCondensed()
        );

    }

    f.printf("%15s %9.7f %9.7f\n",
        "Summ:",
        sum1,
        sum2
    );
    
}

// Close the file
f.close();
