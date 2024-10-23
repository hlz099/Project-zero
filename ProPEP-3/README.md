ProPep traces back to the mid 1980’s and a group of people at Martin Marietta in Orlando, Florida. 
It was a state of the art DOS program with very little documentation. 
Its database, PEPCODE.DAF, had its last official update on September 29th, 1986 by John Cummingham. 
Since that time there have been a multitude of undocumented updated versions.

In the early 1990’s Art Lekstutis, of New York, created a Graphical Interface to make it easier for the average user to run. 
GUIPEP ran in Windows as a frontend for the “original” PROPEP. 
The current version, 0.04 alpha, is dated November 17, 1996. 
Art has ceased any further evolution of GUIPEP.

In 2012 NASSA member Dave Cooper, after completing NASSA’s TC LOGGER USB Data Acquisition Project, wanted to create a program that would characterize Propellant fired on the TC LOGGER System. 
ProPep was required for the calculation of some pertinent data required for the characterization process.

PROPEP 3 was begun to convert the original DOS based PROPEP to a true windows based program which would easily install and run on all the Windows based systems from Windows XP to Windows 7 (32 and 64 bit) and hopefully beyond.

Once the initial DOS based PROPEP, written in FORTRAN, was converted to •Net, NASSA members began to ask that other things be added to aid in the characterization of propellants. 
This included calculation of the values of a and n from BATES grain test burns and the ability to create a file which could be read by BURNSIM to directly input the propellant information into that program.

PROPEP 3 contains the process of the original ProPep and will actually provide a ProPep Report the same as the original ProPep. 
While this Report is an exact duplicate of the original hard to understand report, and of value to only the most versed propellant scientist, it is not really needed by the user and remains only to provide data on the propellant formulation.

PROPEP 3 was designed to read TC LOGGER USB Files however if you collected your propellant data using another system you have the option to manually enter that data and receive the same end results.


PROPEP 3 Version 1.0.3 UPDATE May 18, 2016
Fixed 2 problems created while translating ProPEP from FORTRAN to Visual Basic.
• The program would end abruptly when an element was not found in the JANNAF.DAF file.

• The program would not end with a report of the element which caused the problem.


PROPEP 3 Version 1.0.2 UPDATE October 10, 2015
If you are a current user of PROPEP 3 you should save a copy of your current DAF file. Rename it to be safe. The new DAF file has new chemicals and will write over your old DAF. You can always bring back your renamed DAF version, if not satisfied with the new one, or just re-enter your personal data. All other files will NOT be affected. Current users can update by clicking HERE.  The latest version of PROPEP 3 presents:

• Added the ability to use 4 grains in a test motor

• Corrected an error in the PEPCODED.DAF file to correctly characterize Lectithin thanks to Dr. Martin W. Weiser
