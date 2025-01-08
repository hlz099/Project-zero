exec helper.sce

rpa_reset();

// Load the script from file test.js
script = rpa_load_script("test.js");

// Execute the loaded script
[res,code] = rpa_execute(script);

// Get the values "Combustion temperature [K]" and "mass fraction of condensed species"
// from the objects "e" and "d" created in server context
[T,code] = rpa_execute("e.getT(''K'')");
[z,code] = rpa_execute("d.getZ()");

// Alternative way to to results from scripting server:
[res,code] = rpa_execute("printf(''[%f, %f]'', e.getT(''K''), d.getZ())");
T = res(1);
z = res(2);


