source helper.m

rpa_reset();

s=rpa_load_script("test.js");

rpa_execute(s);

T = rpa_execute("e.getT('K')");
z = rpa_execute("d.getZ()");

