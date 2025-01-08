
//******************************************
// Testing in-memory config...

// This will create an empty configuration directly in memory
test1_cfg = ConstructConfigFile();
assert_checkequal(Pointer_isConfigFile(test1_cfg), %T);

// Convert empty configuration to string
s1 = ConfigFile_toString(test1_cfg);
assert_checkequal(ConfigFile_getPath(test1_cfg), "");

// Empty configuration converted to JSON string:
// {"HEX_Options":{"freezeOutTemperature":{"unit":"K","value":900},"type":"exact method"},"application":"RPA-C","combustionConditions":{"pressure":{"unit":"MPa","value":20.7}},"combustionOptionalConditions":[],"generalOptions":{"ions":false,"multiphase":true},"info":"","ingredients":[],"name":"","version":2}
assert_checkequal("{""HEX_Options"":{""freezeOutTemperature"":{""unit"":""K"",""value"":900},""type"":""exact method""},""application"":""RPA-C"",""combustionConditions"":{""pressure"":{""unit"":""MPa"",""value"":20.7}},""combustionOptionalConditions"":[],""generalOptions"":{""ions"":false,""multiphase"":true},""info"":"""",""ingredients"":[],""name"":"""",""version"":2}", s1);

// Convert JSON to Scilab structure
x1 = fromJSON(s1);
// Empty configuration as Scilab structure:
//  x  =
// HEX_Options: struct with fields:
//     freezeOutTemperature: struct with fields:
//         unit = "K"
//         value = 900
//     type = "exact method"
// application = "RPA-C"
// combustionConditions: struct with fields:
//     pressure: struct with fields:
//         unit = "MPa"
//         value = 20.7
// combustionOptionalConditions = []
// generalOptions: struct with fields:
//     ions = %f
//     multiphase = %t
// info = ""
// ingredients = []
// name = ""
// version = 2
assert_checkequal(x1.application, "RPA-C");
assert_checkequal(x1.version, 2);
assert_checkequal(x1.name, "");
assert_checkequal(x1.info, "");
assert_checkequal(x1.ingredients, []);
assert_checkequal(x1.combustionConditions.pressure.value, 20.7);
assert_checkequal(x1.combustionConditions.pressure.unit, "MPa");
assert_checkequal(x1.combustionOptionalConditions, []);
assert_checkequal(x1.generalOptions.ions, %F);
assert_checkequal(x1.generalOptions.multiphase, %T);
assert_checkequal(x1.HEX_Options.type, "exact method");
assert_checkequal(x1.HEX_Options.freezeOutTemperature.value, 900);
assert_checkequal(x1.HEX_Options.freezeOutTemperature.unit, "K");

//******************************************
// Testing in-memory config, creating it from Scilab structure via JSON...

test2_cfg_struct.application = "RPA-C";
test2_cfg_struct.version = 2;
test2_cfg_struct.name = "test2";
test2_cfg_struct.info = "";
test2_cfg_struct.ingredients = [];
test2_cfg_struct.combustionConditions.pressure.value = 20.7;
test2_cfg_struct.combustionConditions.pressure.unit = "MPa";
test2_cfg_struct.combustionOptionalConditions = [];
test2_cfg_struct.generalOptions.ions = %F;
test2_cfg_struct.generalOptions.multiphase = %T;
test2_cfg_struct.HEX_Options.type = "exact method";
test2_cfg_struct.HEX_Options.freezeOutTemperature.value = 900;
test2_cfg_struct.HEX_Options.freezeOutTemperature.unit = "K";

s2 = toJSON(test2_cfg_struct)

test2_cfg = ConstructConfigFile();
ConfigFile_fromString(test2_cfg, s2);
assert_checkequal(ConfigFile_getPath(test2_cfg), "");

s2 = ConfigFile_toString(test2_cfg);
// Created configuration converted to JSON string again:
// {"HEX_Options":{"freezeOutTemperature":{"unit":"K","value":900},"type":"exact method"},"application":"RPA-C","combustionConditions":{"pressure":{"unit":"MPa","value":20.7}},"combustionOptionalConditions":[],"generalOptions":{"ions":false,"multiphase":true},"info":"","ingredients":[],"name":"test2","version":2}
assert_checkequal("{""HEX_Options"":{""freezeOutTemperature"":{""unit"":""K"",""value"":900},""type"":""exact method""},""application"":""RPA-C"",""combustionConditions"":{""pressure"":{""unit"":""MPa"",""value"":20.7}},""combustionOptionalConditions"":[],""generalOptions"":{""ions"":false,""multiphase"":true},""info"":"""",""ingredients"":[],""name"":""test2"",""version"":2}", s2);

// Convert JSON to Scilab structure
x2 = fromJSON(s2);
// Created configuration as Scilab structure:
//  x  =
// HEX_Options: struct with fields:
//     freezeOutTemperature: struct with fields:
//         unit = "K"
//         value = 900
//     type = "exact method"
// application = "RPA-C"
// combustionConditions: struct with fields:
//     pressure: struct with fields:
//         unit = "MPa"
//         value = 20.7
// combustionOptionalConditions = []
// generalOptions: struct with fields:
//     ions = %f
//     multiphase = %t
// info = ""
// ingredients = []
// name = "test2"
// version = 2
assert_checkequal(x2.application, "RPA-C");
assert_checkequal(x2.version, 2);
assert_checkequal(x2.name, "test2");
assert_checkequal(x2.info, "");
assert_checkequal(x2.ingredients, []);
assert_checkequal(x2.combustionConditions.pressure.value, 20.7);
assert_checkequal(x2.combustionConditions.pressure.unit, "MPa");
assert_checkequal(x2.combustionOptionalConditions, []);
assert_checkequal(x2.generalOptions.ions, %F);
assert_checkequal(x2.generalOptions.multiphase, %T);
assert_checkequal(x2.HEX_Options.type, "exact method");
assert_checkequal(x2.HEX_Options.freezeOutTemperature.value, 900);
assert_checkequal(x2.HEX_Options.freezeOutTemperature.unit, "K");

// Write to the config file for the next test
ConfigFile_write(test2_cfg, "test3.cfg");
assert_checkequal(ConfigFile_getPath(test2_cfg), "");

//******************************************
// Test config file created at previous step

test3_cfg = ConstructConfigFile();
ConfigFile_read(test3_cfg, "test3.cfg");
assert_checkequal(ConfigFile_getPath(test3_cfg), "test3.cfg");

s3 = ConfigFile_toString(test3_cfg);
assert_checkequal("{""HEX_Options"":{""freezeOutTemperature"":{""unit"":""K"",""value"":900},""type"":""exact method""},""application"":""RPA-C"",""combustionConditions"":{""pressure"":{""unit"":""MPa"",""value"":20.7}},""combustionOptionalConditions"":[],""generalOptions"":{""ions"":false,""multiphase"":true},""info"":"""",""ingredients"":[],""name"":""test2"",""version"":2}", s3);

x3 = fromJSON(s3);
assert_checkequal(x3.application, "RPA-C");
assert_checkequal(x3.version, 2);
assert_checkequal(x3.name, "test2");
assert_checkequal(x3.info, "");
assert_checkequal(x3.ingredients, []);
assert_checkequal(x3.combustionConditions.pressure.value, 20.7);
assert_checkequal(x3.combustionConditions.pressure.unit, "MPa");
assert_checkequal(x3.combustionOptionalConditions, []);
assert_checkequal(x3.generalOptions.ions, %F);
assert_checkequal(x3.generalOptions.multiphase, %T);
assert_checkequal(x3.HEX_Options.type, "exact method");
assert_checkequal(x3.HEX_Options.freezeOutTemperature.value, 900);
assert_checkequal(x3.HEX_Options.freezeOutTemperature.unit, "K");

//******************************************

// Continue modification of test2 via Scilab structure
test2_cfg_struct.name = "test4";
test2_cfg_struct.info = "Updated config";

// Update the configuration
s2 = toJSON(test2_cfg_struct)
ConfigFile_fromString(test2_cfg, s2);

// Write to the config file for the next test
ConfigFile_write(test2_cfg, "test4.cfg");
assert_checkequal(ConfigFile_getPath(test2_cfg), "");

test4_cfg = ConstructConfigFile();
ConfigFile_read(test4_cfg, "test4.cfg");
assert_checkequal(ConfigFile_getPath(test4_cfg), "test4.cfg");

s4 = ConfigFile_toString(test4_cfg);
x4 = fromJSON(s4);
assert_checkequal(x4.application, "RPA-C");
assert_checkequal(x4.version, 2);
assert_checkequal(x4.name, "test4");
assert_checkequal(x4.info, "Updated config");


//******************************************


test5_cfg_struct.application = "RPA-C";
test5_cfg_struct.version = 2;
test5_cfg_struct.name = "test5";
test5_cfg_struct.info = "Test 5 info";

test5_cfg_ing1_struct.massFraction = 0.7;
test5_cfg_ing1_struct.name = "H2O2(L)";
test5_cfg_ing2_struct.massFraction = 0.3;
test5_cfg_ing2_struct.name = "Al(cr)";
test5_cfg_struct.ingredients = [test5_cfg_ing1_struct, test5_cfg_ing2_struct];

test5_cfg_struct.combustionConditions.pressure.value = 20.7;
test5_cfg_struct.combustionConditions.pressure.unit = "MPa";

test5_cfg_opt1_struct.pressure.value = 10;
test5_cfg_opt1_struct.pressure.unit = "MPa";
test5_cfg_opt2_struct.pressure.value = 25;
test5_cfg_opt2_struct.pressure.unit = "MPa";
test5_cfg_struct.combustionOptionalConditions = [test5_cfg_opt1_struct, test5_cfg_opt2_struct];

test5_cfg_struct.generalOptions.ions = %F;
test5_cfg_struct.generalOptions.multiphase = %T;
test5_cfg_struct.HEX_Options.type = "exact method";
test5_cfg_struct.HEX_Options.freezeOutTemperature.value = 900;
test5_cfg_struct.HEX_Options.freezeOutTemperature.unit = "K";

s5 = toJSON(test5_cfg_struct)

test5_cfg = ConstructConfigFile();
ConfigFile_fromString(test5_cfg, s5);
x5 = fromJSON(ConfigFile_toString(test5_cfg));
assert_checkequal(x5.application, "RPA-C");
assert_checkequal(x5.version, 2);
assert_checkequal(x5.name, "test5");
assert_checkequal(x5.info, "Test 5 info");
assert_checkequal(length(x5.ingredients), 2);
assert_checkequal(x5.ingredients(1).name, "H2O2(L)");
assert_checkequal(x5.ingredients(1).massFraction, 0.7);
assert_checkequal(x5.ingredients(2).name, "Al(cr)");
assert_checkequal(x5.ingredients(2).massFraction, 0.3);
assert_checkequal(x5.combustionConditions.pressure.value, 20.7);
assert_checkequal(x5.combustionConditions.pressure.unit, "MPa");
assert_checkequal(length(x5.combustionOptionalConditions), 2);
assert_checkequal(x5.combustionOptionalConditions(1).pressure.value, 10);
assert_checkequal(x5.combustionOptionalConditions(1).pressure.unit, "MPa");
assert_checkequal(x5.combustionOptionalConditions(2).pressure.value, 25);
assert_checkequal(x5.combustionOptionalConditions(2).pressure.unit, "MPa");
assert_checkequal(x5.generalOptions.ions, %F);
assert_checkequal(x5.generalOptions.multiphase, %T);
assert_checkequal(x5.HEX_Options.type, "exact method");
assert_checkequal(x5.HEX_Options.freezeOutTemperature.value, 900);
assert_checkequal(x5.HEX_Options.freezeOutTemperature.unit, "K");


//******************************************
// Test API functions

// Reload the file again

// File version
assert_checkequal(ConfigFile_getVersion(test5_cfg), 2);

// Case name
assert_checkequal(ConfigFile_getName(test5_cfg), "test5");
ConfigFile_setName(test5_cfg, "test5 updated")
x5 = fromJSON(ConfigFile_toString(test5_cfg));
assert_checkequal(x5.name, "test5 updated");
assert_checkequal(ConfigFile_getName(test5_cfg), "test5 updated");

// Case info
assert_checkequal(ConfigFile_getInfo(test5_cfg), "Test 5 info");
ConfigFile_setInfo(test5_cfg, "Test 5 info updated")
x5 = fromJSON(ConfigFile_toString(test5_cfg));
assert_checkequal(x5.info, "Test 5 info updated");
assert_checkequal(ConfigFile_getInfo(test5_cfg), "Test 5 info updated");

// Ingredients
ingredients_cfg = ConfigFile_getIngredients(test5_cfg)
disp(ingredients_cfg)
assert_checkequal(Ingredients_getSize(ingredients_cfg), 2);

assert_checkequal(Ingredients_isOmitAtomsER(ingredients_cfg), %F);
Ingredients_setOmitAtomsER(ingredients_cfg, "Cu,Fe");
assert_checkequal(Ingredients_isOmitAtomsER(ingredients_cfg), %T);
assert_checkequal(Ingredients_getOmitAtomsER(ingredients_cfg), "Cu,Fe");

// Test Ingredient (Component)
comp_cfg = Ingredients_getComponent(ingredients_cfg, 0);
disp(comp_cfg)
assert_checkequal(Component_getName(comp_cfg), "H2O2(L)");
Component_setName(comp_cfg, "O2(L)");
assert_checkequal(Component_getName(comp_cfg), "O2(L)");

assert_checkequal(Component_getMf(comp_cfg), 0.7);
Component_setMf(comp_cfg, 0.75);
assert_checkequal(Component_getMf(comp_cfg), 0.75);

assert_checkequal(Component_isT(comp_cfg), %F);
assert_checkequal(Component_isP(comp_cfg), %F);
assert_checkequal(Component_isDensity(comp_cfg), %F);

Component_setT(comp_cfg, -183.15, "C");
assert_checkequal(Component_isT(comp_cfg), %T);
assert_checkalmostequal(Component_getT(comp_cfg, "K"), 90);
assert_checkalmostequal(Component_getT(comp_cfg, "C"), -183.15);

Component_setT(comp_cfg, 90, "K");
assert_checkequal(Component_isT(comp_cfg), %T);
assert_checkalmostequal(Component_getT(comp_cfg, "K"), 90);
assert_checkalmostequal(Component_getT(comp_cfg, "C"), -183.15);

Component_setP(comp_cfg, 0.101325, "MPa");
assert_checkequal(Component_isP(comp_cfg), %T);
assert_checkalmostequal(Component_getP(comp_cfg, "atm"), 1);
assert_checkalmostequal(Component_getP(comp_cfg, "Pa"), 101325);
assert_checkalmostequal(Component_getP(comp_cfg, "MPa"), 0.101325);

Component_setDensity(comp_cfg, 1, "g/cm^3");
assert_checkequal(Component_isDensity(comp_cfg), %T);
assert_checkalmostequal(Component_getDensity(comp_cfg, "g/cm^3"), 1);
assert_checkalmostequal(Component_getDensity(comp_cfg, "kg/m^3"), 1000);

Component_setDensity(comp_cfg, 1, "kg/m^3");
assert_checkequal(Component_isDensity(comp_cfg), %T);
assert_checkalmostequal(Component_getDensity(comp_cfg, "g/cm^3"), 0.001);
assert_checkalmostequal(Component_getDensity(comp_cfg, "kg/m^3"), 1);

comp_cfg = Ingredients_getComponent(ingredients_cfg, 1);
disp(comp_cfg)
assert_checkequal(Component_getName(comp_cfg), "Al(cr)");
assert_checkequal(Component_getMf(comp_cfg), 0.3);

//comp_cfg = Ingredients_getComponent(ingredients_cfg, 2); -- Invalid index, should produce Scilab error
assert_checkerror("comp_cfg = Ingredients_getComponent(ingredients_cfg, 2)", "SWIG/Scilab: RuntimeError: Runtime error")

// Constructor of empty component
comp_cfg = ConstructComponent();
assert_checkequal(Component_getName(comp_cfg), "");
assert_checkequal(Component_getMf(comp_cfg), 0);
assert_checkequal(Component_isT(comp_cfg), %F);
assert_checkequal(Component_isP(comp_cfg), %F);
assert_checkequal(Component_isDensity(comp_cfg), %F);

// Constructor of component
comp_cfg = ConstructComponent("Mg(cr)", 0.1);
assert_checkequal(Component_getName(comp_cfg), "Mg(cr)");
assert_checkequal(Component_getMf(comp_cfg), 0.1);
assert_checkequal(Component_isT(comp_cfg), %F);
assert_checkequal(Component_isP(comp_cfg), %F);
assert_checkequal(Component_isDensity(comp_cfg), %F);

// Test adding created component
Ingredients_addComponent(ingredients_cfg, comp_cfg);
assert_checkequal(Ingredients_getSize(ingredients_cfg), 3);
comp_cfg_2 = Ingredients_getComponent(ingredients_cfg, 2);
disp(comp_cfg_2)
assert_checkerror("comp_cfg = Ingredients_getComponent(ingredients_cfg, 3)", "SWIG/Scilab: RuntimeError: Runtime error")
assert_checkequal(Component_getName(comp_cfg_2), "Mg(cr)");
assert_checkequal(Component_getMf(comp_cfg_2), 0.1);

x5 = fromJSON(ConfigFile_toString(test5_cfg));
disp(x5)

// Test reset
Ingredients_reset(ingredients_cfg);
assert_checkequal(Ingredients_getSize(ingredients_cfg), 0);
assert_checkerror("comp_cfg = Ingredients_getComponent(ingredients_cfg, 0)", "SWIG/Scilab: RuntimeError: Runtime error")

// General options
gopt = ConfigFile_getGeneralOptions(test5_cfg);

GeneralOptions_setMultiphase(gopt, %F);
assert_checkequal(GeneralOptions_isMultiphase(gopt), %F);
GeneralOptions_setMultiphase(gopt, %T);
assert_checkequal(GeneralOptions_isMultiphase(gopt), %T);

GeneralOptions_setIons(gopt, %F);
assert_checkequal(GeneralOptions_isIons(gopt), %F);
GeneralOptions_setIons(gopt, %T);
assert_checkequal(GeneralOptions_isIons(gopt), %T);

// Combustion Conditions

cc = ConfigFile_getCombustionConditions(test5_cfg);
assert_checkalmostequal(CombustionConditions_getP(cc, "MPa"), 20.7);
assert_checkalmostequal(CombustionConditions_getP(cc, "atm"), 204.29312, 0.001);
CombustionConditions_setP(cc, 10, "MPa");
assert_checkalmostequal(CombustionConditions_getP(cc, "MPa"), 10);
assert_checkalmostequal(CombustionConditions_getP(cc, "atm"), 98.692327, 0.001);
CombustionConditions_setP(cc, 98.692327, "atm");
assert_checkalmostequal(CombustionConditions_getP(cc, "MPa"), 10, 0.001);
assert_checkalmostequal(CombustionConditions_getP(cc, "atm"), 98.692327);

assert_checkequal(CombustionConditions_isT(cc), %F);
CombustionConditions_setT(cc, 1400, "K");
assert_checkalmostequal(CombustionConditions_getT(cc, "K"), 1400);
assert_checkalmostequal(CombustionConditions_getT(cc, "C"), 1126.85);
CombustionConditions_setT(cc, 1126.85, "C");
assert_checkalmostequal(CombustionConditions_getT(cc, "K"), 1400);
assert_checkalmostequal(CombustionConditions_getT(cc, "C"), 1126.85);
assert_checkequal(CombustionConditions_isT(cc), %T);

x5 = fromJSON(ConfigFile_toString(test5_cfg));
disp(x5)

CombustionConditions_deleteT(cc);
assert_checkequal(CombustionConditions_isT(cc), %F);

assert_checkequal(CombustionConditions_getOmitProductsSize(cc), 0);

// Combustion Optional Conditions
assert_checkequal(ConfigFile_getCombustionOptionalConditionsSize(test5_cfg), 2);
assert_checkerror("copt = ConfigFile_getCombustionOptionalConditions(test5_cfg, 2)", "SWIG/Scilab: RuntimeError: Runtime error")

copt = ConfigFile_getCombustionOptionalConditions(test5_cfg, 0);
assert_checkalmostequal(CombustionConditions_getP(copt, "MPa"), 10);
assert_checkalmostequal(CombustionConditions_getP(copt, "atm"), 98.692327, 0.001);
CombustionConditions_setP(copt, 98.692327, "atm");
assert_checkalmostequal(CombustionConditions_getP(copt, "MPa"), 10, 0.001);
assert_checkalmostequal(CombustionConditions_getP(copt, "atm"), 98.692327);

assert_checkequal(CombustionConditions_isT(copt), %F);
CombustionConditions_setT(copt, 1400, "K");
assert_checkalmostequal(CombustionConditions_getT(copt, "K"), 1400);
assert_checkalmostequal(CombustionConditions_getT(copt, "C"), 1126.85);
CombustionConditions_setT(copt, 1126.85, "C");
assert_checkalmostequal(CombustionConditions_getT(copt, "K"), 1400);
assert_checkalmostequal(CombustionConditions_getT(copt, "C"), 1126.85);
assert_checkequal(CombustionConditions_isT(copt), %T);

CombustionConditions_deleteT(copt);
assert_checkequal(CombustionConditions_isT(copt), %F);

//ConfigFile_setCombustionOptionalConditions(test5_cfg)

// Add new optional conditions with default params (Pressure 20.7 MPa, no Temperature)
copt_2= ConfigFile_setCombustionOptionalConditions(test5_cfg);
assert_checkalmostequal(CombustionConditions_getP(copt_2, "MPa"), 20.7);
assert_checkequal(CombustionConditions_isT(copt_2), %F);

assert_checkequal(ConfigFile_getCombustionOptionalConditionsSize(test5_cfg), 3);
assert_checkerror("copt = ConfigFile_getCombustionOptionalConditions(test5_cfg, 3)", "SWIG/Scilab: RuntimeError: Runtime error")


ConfigFile_clearCombustionOptionalConditionsList(test5_cfg);
assert_checkequal(ConfigFile_getCombustionOptionalConditionsSize(test5_cfg), 0);


// Hex Conditions

hexc = ConfigFile_getHexConditions(test5_cfg);

HEXConditions_setType(hexc, "invalid");
assert_checkequal(HEXConditions_getType(hexc), "none");

HEXConditions_setType(hexc, "none");
assert_checkequal(HEXConditions_getType(hexc), "none");

HEXConditions_setType(hexc, "inert diluent method");
assert_checkequal(HEXConditions_getType(hexc), "inert diluent method");

HEXConditions_setType(hexc, "exact method");
assert_checkequal(HEXConditions_getType(hexc), "exact method");

// Default value already set
assert_checkequal(HEXConditions_isFreezeOutTemperature(hexc), %T);
assert_checkalmostequal(HEXConditions_getFreezeOutTemperature(hexc, "K"), 900);
assert_checkalmostequal(HEXConditions_getFreezeOutTemperature(hexc, "C"), 626.85);
HEXConditions_setFreezeOutTemperature(hexc, 1400, "K");
assert_checkalmostequal(HEXConditions_getFreezeOutTemperature(hexc, "K"), 1400);
assert_checkalmostequal(HEXConditions_getFreezeOutTemperature(hexc, "C"), 1126.85);
HEXConditions_deleteFreezeOutTemperature(hexc);
assert_checkequal(HEXConditions_isFreezeOutTemperature(hexc), %F);

assert_checkequal(HEXConditions_isAssignedLoadDensity(hexc), %F);
HEXConditions_setAssignedLoadDensity(hexc, 1, "g/cm^3");
assert_checkequal(HEXConditions_isAssignedLoadDensity(hexc), %T);
assert_checkalmostequal(HEXConditions_getAssignedLoadDensity(hexc, "g/cm^3"), 1);
assert_checkalmostequal(HEXConditions_getAssignedLoadDensity(hexc, "kg/m^3"), 1000);
HEXConditions_setAssignedLoadDensity(hexc, 1, "kg/m^3");
assert_checkequal(HEXConditions_isAssignedLoadDensity(hexc), %T);
assert_checkalmostequal(HEXConditions_getAssignedLoadDensity(hexc, "g/cm^3"), 0.001);
assert_checkalmostequal(HEXConditions_getAssignedLoadDensity(hexc, "kg/m^3"), 1);
HEXConditions_deleteAssignedLoadDensity(hexc);
assert_checkequal(HEXConditions_isAssignedLoadDensity(hexc), %F);

assert_checkequal(HEXConditions_getReplaceProductsSize(hexc), 0);
HEXConditions_addReplaceProduct(hexc, "H2O", "H2O(L)");
assert_checkequal(HEXConditions_getReplaceProductsSize(hexc), 1);
assert_checkequal(HEXConditions_getReplaceProductKey(hexc, 0), "H2O");
assert_checkequal(HEXConditions_getReplaceProductValue(hexc, 0), "H2O(L)");
HEXConditions_clearReplaceProducts(hexc);
assert_checkequal(HEXConditions_getReplaceProductsSize(hexc), 0);

assert_checkequal(HEXConditions_getIncludeProductsSize(hexc), 0);
HEXConditions_addIncludeProduct(hexc, "H2O");
assert_checkequal(HEXConditions_getIncludeProductsSize(hexc), 1);
assert_checkequal(HEXConditions_getIncludeProduct(hexc, 0), "H2O");
HEXConditions_clearIncludeProducts(hexc);
assert_checkequal(HEXConditions_getIncludeProductsSize(hexc), 0);

assert_checkequal(HEXConditions_getOmitProductsSize(hexc), 0);
HEXConditions_addOmitProduct(hexc, "H2O");
assert_checkequal(HEXConditions_getOmitProductsSize(hexc), 1);
assert_checkequal(HEXConditions_getOmitProduct(hexc, 0), "H2O");
HEXConditions_clearOmitProducts(hexc);
assert_checkequal(HEXConditions_getOmitProductsSize(hexc), 0);

// Testing completed!

