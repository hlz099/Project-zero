# NASAUCPJavaStandalone
Port of various NASA projects from a Java applets to standalone Java applications.

FoilSim III Student Version 1.5a
https://www.grc.nasa.gov/www/k-12/airplane/foil3.html

TunnelSys - Tunnel Test Applet Version 1.0a
https://www.grc.nasa.gov/www/k-12/airplane/tunwtest.html

Interactive Wright 1901 Wind Tunnel
https://wright.nasa.gov/airplane/tunnlint.html

Mach and Speed of Sound Calculator
https://www.grc.nasa.gov/www/k-12/airplane/machu.html

Isentropic Flow Calculator
https://www.grc.nasa.gov/www/k-12/airplane/ieisen.html

ShockSim Version 1.3e Shock Wave Simulator
https://www.grc.nasa.gov/www/k-12/airplane/shock.html

Supersonic Cone Simulator
https://www.grc.nasa.gov/www/k-12/airplane/coneflow.html

ShockModeler Version 1.3a Multiple Shock Wave Simulator
https://www.grc.nasa.gov/www/k-12/airplane/mshock.html

Method of Characteristics analysis Nozzle Simulation
https://www.grc.nasa.gov/www/k-12/airplane/mocnoz.html

Interactive Nozzle Simulator
https://www.grc.nasa.gov/www/k-12/airplane/ienzl.html

Supersonic Flows Simulator
https://www.grc.nasa.gov/www/k-12/airplane/supersim.html

EngineSimU Brayton Cycle analysis of a turbine engine or ramjet
https://www.grc.nasa.gov/www/k-12/airplane/engsimu.html

FoilSimU III simulator that performs a Kutta-Joukowski analysis
https://www.grc.nasa.gov/www/k-12/UndergradProgs/index.htm

While it's easy to add a main method to an applet, and add the applet panel to a Frame, and thus make it runnable and displayable as an application, this is not generally sufficient. The reason is that browsers provide some extra infrastucture for applets to use. To help with this, Jef Poskanzer has written a very useful adapter class called MainFrame which helps provide this infrastructure. 
-------------
To use them, we need to install Java for windows or linux. In the downloaded files, notice the two files Driver.class and Driver.java. To run these programs, open terminal (linux) or powerShell, cmd (windows) and run the command: java Driver. By default, all programs will run at the same time. To run only a certain program, for example Nozzle, open the Driver.java file, in public … {} comment all lines that do not contain the word nozzle by adding a double slash // at the beginning of the line. Rename the public class to NozzleProg, rename the file to NozzleProg.java, recompile this file with the command: javac NozzleProg.java (or javac -Xlint NozzleProg.java). We will have a new file NozzleProg.class. To run it use NozzleProg java command. Likewise for the remaining programs.