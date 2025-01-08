source helper.m

rpa_reset();

# Load RPA-C JS helper script and execute it on server
s_helper=rpa_load_script("helper.js");
rpa_execute(s_helper);

# Using JS helper, prepare mixture ans store it in server context as variable "mix"
rpa_execute("mix = prepareMixture(['NH4CLO4(cr)', 0.7], ['AL(cr)', 0.2], ['HTPB+Curative', 0.1])");

p_array = [1, 2, 3, 4, 5, 7.5, 10, 12.5, 15, 17.5, 20, 22.5, 25, 27.5, 30]; # MPa
T_array = []; # Vector to collect calculated temperatures

for p = p_array
    # Result "res" is stored in the server context
    # and can be reused to obtain calculated values
    rpa_execute(sprintf("res = solve(mix, %f, 'MPa')", p));
    # Obtain required value from stored result
    T_array(end+1) = str2num(rpa_execute("res.e.getT('K')"));
end

plot(p_array, T_array);
xlabel ("p, MPa");
ylabel ("T, K");
