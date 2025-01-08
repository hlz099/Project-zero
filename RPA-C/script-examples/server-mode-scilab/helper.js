// Example of helper functions which can be used from Scilab or Octave scripts

// Prepare Mixture
function prepareMixture() {
    var mix = new Mixture();
    for (var i=0; i<arguments.length; i++) {
        var arg = arguments[i];
        mix.addSpecies(arg[0], arg[1]);
    }
    return mix;
}

// Solve the combustion problem
function solve(mix, p, p_units) {
    var c = new Combustor(mix, true, true);
    c.setP(p, p_units);
    c.solve(true, false);
    return {"e":c.getEquilibrium(), "d":c.getDerivatives()};
}
