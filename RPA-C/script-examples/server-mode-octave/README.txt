This directory includes the examples to demonstrate the usage of server mode of RPA-S scripting utility from Octave scripts.

To get started, run RPA-S Scripting Utility as a server:

    RPA-C> ./RPA-C-Script.exe --server

Start Octave (in example below - command-line interface, but UI version can be used as well):

    > octave

In Octave console, execute provided test script "test_server_1.m":

    octave:1> source test_server_1.m

To test it, check variables T and Z:

    octave:2> T
        T = 3767.5526092485566
    octave:3> z
        z = 0.3384350541239415

In Octave console, execute provided test script "test_server_2.m":

    octave:1> source test_server_2.m

When executed correctly and plotting from Octave is allowed, the windows with diagram will be opened.
To test the correctness of execution, check the variable T_array:

    octave:2> T_array
        T_array = 3422.4   3506.6   3555.0   3588.8   3614.6   3660.4   3692.0   3715.9   3734.9   3750.7   3764.1   3775.8   3786.0   3795.1   3803.3

