Thrust Curve Tracer
TCTracer is a freely available Java application that makes creation of rocket motor data files quick and easy. 
If you have an image of the thrust curve graph of a motor plus some standard information about it, you can use it to create a motor file simply and accurately.

In simplest terms, TCtracer opens an image file of the motor's thrust curve and lets you draw right on that image. 
After drawing the thrust curve, you input some standard data on the motor and it will write out a RASP format motor data file. 
TCTracer is written as a Java application and should run on all modern operating systems. 
(I have tested it on Windows XP, Mac OS X and Linux.)


--------------------------------------------------------------------------
There are five steps in using TCtracer to make a motor data file:

1.Open Image loads an image file of the thrust curve to be traced.
2.Setup Grid allows you to specify the grid values and drag the edges.
3.Draw Points is used to draw the data points directly on the graph image.
4.Motor Info brings up a dialog to enter basic information on the motor.
5.Save Data saves the resulting RASP format simulator file.