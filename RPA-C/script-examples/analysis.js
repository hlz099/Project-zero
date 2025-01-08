/***************************************************
 RPA-C - tool for the thermodynamic analysis.
  
 This script loads existing configuration file,
 solves the configured problem and prints out the results.
****************************************************/

// Load configuration file
c = new ConfigFile("examples/HMX.cfg");
c.read();

// Create and run combustion analysis
ca = new CombustionAnalysis();
ca.run(c);

if (ca.getCombustorsListSize()>0) {
    printf("Initial mixture:\n");
    ca.getMixture().print();

    tUnit = "K";
    printf("T   = %10.5f %s\n", ca.getCombustor(0).getEquilibrium().getT(tUnit), tUnit);

    hexUnit = "kJ/kg";
    printf("HEX = %10.5f %s\n", ca.getHEX(hexUnit), hexUnit);
} else {
    printf("Could not solve!\n");
}
