//Lambert solver
//Dario Izzo 2008
//modified for EMTG by Jacob Englander 2010-2011

#include <math.h>
#include "Astrodynamics.h"
#include "EMTG_math.h"

using namespace EMTG;

namespace EMTG { namespace Astrodynamics {




//TODO negative time in lambert solver should not crash the program; rather it should return an error code
void Lambert (const double *r1_in, const double *r2_in, double t, const double mu, //INPUT
	           const bool lw, const int N, //INPUT
	           double *v1, double *v2)//OUTPUT
{
	double	V,T,
    r2_mod = 0.0,    // R2 module
    dot_prod = 0.0, // dot product
    c,		        // non-dimensional chord
    s,		        // non dimesnional semi-perimeter
    am,		        // minimum energy ellipse semi major axis
    lambda,	        //lambda parameter defined in Battin's Book

    x,x1,x2,y1,y2,x_new=0,y_new,err,alfa,beta,psi,eta,eta2,sigma1,vr1,vt1,vt2,vr2,R=0.0;
	double a, p, theta;
	double inn1 = 0.0, inn2 = 0.0;
	const double tolerance = 1.0e-6;
	int i_count, i, branch = 0;
	double r1[3], r2[3], r2_unitv[3];
	double ih_dum[3], ih[3], dum[3];

	// Increasing the tolerance does not bring any advantage as the
	// precision is usually greater anyway (due to the rectification of the tof
	// graph) except near particular cases such as parabolas in which cases a
	// lower precision allow for usual convergence.

	if (t <= 0)
    {
		//cout << "ERROR in Lambert Solver: Negative Time in input." << endl;
        return;
    }

	for (i = 0; i < 3; ++i)
    {
      r1[i] = r1_in[i];
      r2[i] = r2_in[i];
	  R += r1[i]*r1[i];
    }

	R = sqrt(R);
	V = sqrt(mu/R);
	T = R/V;

    // working with non-dimensional radii and time-of-flight
	t /= T;
	for (i = 0;i <3;++i)  // r1 dimension is 3
    {
		r1[i] /= R;
		r2[i] /= R;
		r2_mod += r2[i]*r2[i];
    }

	// Evaluation of the relevant geometry parameters in non dimensional units
	r2_mod = sqrt(r2_mod);

	for (i = 0;i < 3;++i)
      dot_prod += (r1[i] * r2[i]);

	theta = acos(dot_prod/r2_mod);

	if (lw)
		theta=2*acos(-1.0)-theta;

	c = sqrt(1 + r2_mod*(r2_mod - 2.0 * cos(theta)));
	s = (1 + r2_mod + c)/2.0;
	am = s/2.0;
	lambda = sqrt (r2_mod) * cos (theta/2.0)/s;


   // We start finding the log(x+1) value of the solution conic:
    // NO MULTI REV --> (1 SOL)
	if (N == 0)
	{
		inn1=-.5233;    //first guess point
		inn2=.5233;     //second guess point
		x1=log(1+inn1);
		x2=log(1+inn2);
		y1=log(x2tof(inn1,s,c,lw,N))-log(t);
		y2=log(x2tof(inn2,s,c,lw,N))-log(t);

		// Newton iterations
		err=1;
		i_count=0;
		while ((err>tolerance) && (y1 != y2) && i_count<60)
		{
			++i_count;
			x_new=(x1*y2-y1*x2)/(y2-y1);
			y_new=logf(x2tof(expf(x_new)-1,s,c,lw,N))-logf(t); //[MR] Why ...f() functions? Loss of data!
			x1=x2;
			y1=y2;
			x2=x_new;
			y2=y_new;
			err = fabs(x1-x_new);
		}
		x = expf(x_new)-1; //[MR] Same here... expf -> exp
	}
	else
	{
		 //MULTI REV --> (2 SOL) SEPARATING RIGHT AND LEFT BRANCH
		if (branch==0) //left branch
		{
			inn1=-.5234;
			inn2=-.2234;
		}
		else //right branch
		{
			inn1=.7234;
			inn2=.5234;
		}

		x1=tan(inn1*math::PI/2);
		x2=tan(inn2*math::PI/2);
		y1=x2tof(inn1,s,c,lw,N)-t;

		y2=x2tof(inn2,s,c,lw,N)-t;
		err=1;
		i_count=0;

		//Newton Iteration
		while ((err>tolerance) && (y1 != y2) && i_count<60)
		{
			++i_count;
			x_new=(x1*y2-y1*x2)/(y2-y1);
			y_new=x2tof(atan(x_new)*2/math::PI,s,c,lw,N)-t;
			x1=x2;
			y1=y2;
			x2=x_new;
			y2=y_new;
			err=fabs(x1-x_new);
		}
		x=atan(x_new)*2/math::PI;
	}

    // The solution has been evaluated in terms of log(x+1) or tan(x*pi/2), we
    // now need the conic. As for transfer angles near to pi the lagrange
    // coefficient technique goes singular (dg approaches a zero/zero that is
    // numerically bad) we here use a different technique for those cases. When
    // the transfer angle is exactly equal to pi, then the ih unit vector is not
    // determined. The remaining equations, though, are still valid.

	a = am/(1 - x*x);

	// psi evaluation
	if (x < 1)  // ellipse
    {
		beta = 2 * asin (sqrt( (s-c)/(2*a) ));
		if (lw) beta = -beta;
		alfa=2*acos(x);
		psi=(alfa-beta)/2;
		eta2=2*a*(sin(psi)*sin(psi))/s;
		eta=sqrt(eta2);
    }
	else       // hyperbola
    {
		beta = 2*math::asinh(sqrt((c-s)/(2*a)));
		if (lw) beta = -beta;
		alfa = 2*math::acosh(x);
		psi = (alfa-beta)/2;
		eta2 = -2 * a * (sinh(psi)*sinh(psi))/s;
		eta = sqrt(eta2);
    }

	// parameter of the solution
	p = ( r2_mod / (am * eta2) ) * pow (sin (theta/2),2);
	sigma1 = (1/(eta * sqrt(am)) )* (2 * lambda * am - (lambda + x * eta));
	math::cross(r1,r2,ih_dum);
	math::unitv(ih_dum,ih) ;

	if (lw)
    {
		for (i = 0; i < 3;++i)
			ih[i]= -ih[i];
    }

	vr1 = sigma1;
	vt1 = sqrt(p);
	math::cross(ih,r1,dum);

	for (i = 0;i < 3 ;++i)
		v1[i] = vr1 * r1[i] + vt1 * dum[i];

	vt2 = vt1 / r2_mod;
	vr2 = -vr1 + (vt1 - vt2)/tan(theta/2);
	math::unitv(r2,r2_unitv);
	math::cross(ih,r2_unitv,dum);
	for (i = 0;i < 3 ;++i)
		v2[i] = vr2 * r2[i] / r2_mod + vt2 * dum[i];

	for (i = 0;i < 3;++i)
    {
		v1[i] *= V;
		v2[i] *= V;
    }
	a *= R;
	p *= R;
}

double x2tof(const double x,const double s,const double c,const int lw, const int N)
{
	double am,a,alfa,beta;

	am = s/2;
	a = am/(1-x*x);
	if (x < 1)//ellpise
    {
		beta = 2 * asin (sqrt((s - c)/(2*a)));
		if (lw) beta = -beta;
		alfa = 2 * acos(x);
    }
	else
    {
		alfa = 2 * math::acosh(x);
		beta = 2 * math::asinh(sqrt ((s - c)/(-2 * a)));
		if (lw) beta = -beta;
    }

	if (a > 0)
    {
      return (a * sqrt (a)* ( (alfa - sin(alfa)) - (beta - sin(beta)) + N*2*math::PI));
    }
	else
    {
      return (-a * sqrt(-a)*( (sinh(alfa) - alfa) - ( sinh(beta) - beta)) );
    }

}

}}