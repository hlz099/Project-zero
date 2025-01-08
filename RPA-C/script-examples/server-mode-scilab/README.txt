This directory includes the examples to demonstrate the usage of server mode of RPA-S scripting utility from Scilab scripts.

To get started, run RPA-S Scripting Utility as a server:

    RPA-C> ./RPA-C-Script.exe --server

Start Scilab (in example below - UI version, but command-line interface can be used as well):

    > scilab

In Scilab console, execute provided test script "test_server_1.sce":

    --> exec test_server_1.sce

To test it, check variables T and Z:

    --> T
    T  =
        3767.5526

    --> z
    z  =
        0.338435

In Scilab console, execute provided test script "test_server_2.sce":

    --> exec test_server_2.sce

When executed correctly and plotting from Scilab is allowed, the windows with diagram will be opened.
To test the correctness of execution, check the variable T_array:

    --> T_array
    T_array  =
        3422.3686
        3506.6007
        3554.9929
        3588.7636
        3614.566
        3660.4091
        3691.9873
        3715.8614
        3734.9275
        3750.7168
        3764.1357
        3775.7642
        3785.995
        3795.1063
        3803.302

