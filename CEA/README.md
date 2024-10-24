## Introduction

CEA (Chemical Equilibrium with Applications) is an open-source NASA thermodynamics library that contains vital propellant combustion data. Its main use for the propulsion team is to provide emperical data for combustion products and characteristics. Namely, given an input propellant combination, its O/F ratio, and chamber pressure, CEA can calculate the exhaust product molecular mass (M), ratio of specific heats (k), combustion temperature, and an estimated ideal C* efficiency. CEA is given in two forms, a Graphical User Interface (`~/CEAgui`), or as a Windows executable (`~/CEAexec`).
  
## Usage, Installation, and Procedures

### CEAgui/CEAexec

`~/CEAgui` should not be run directly after cloning this repository (avoid pushing to this repository if you have changes to the CEAgui folder). The first step should always be copy and pasting the CEAgui folder into another local working directory. Once CEAgui is in another location, execution should be simple: run `CEAexec-win.bat`. *Make sure you run the .bat (Windows Batch File) version of CEAexec-win.* Of course, this implies that CEAgui only works on a Windows operating system. For further instructions on how to use CEAgui in a technical lens, try this Youtube tutorial: <https://www.youtube.com/watch?v=RbG2n6ClaCs>.

`~/CEAexec` has identical capabilities as CEAgui, but in the form of an interfaceable Windows executable (.exe). There are already Python scripts that will automate the iteration through different O/F ratios and chamber pressures for CEAexec. They can be found inside the `~/CEAexec` directory. *Make sure you run the scripts in this exact order:* 1) `input_generator.py`, `driver_cea2.py`, `output_parser.py`. After the completion of `output_parser.py`, a directory containing .inp, .out, and .csv files should be found with a name corresponding to the parameters that were imputted (e.g. 95% ethanol with nitrous oxide will be titled "ethanol-95-n2o"). The .csv file will be especially useful for MATLAB plotting scripts found elsewhere in this repository.

### Notes

All Python scripts are compatible with Python 3.6+.