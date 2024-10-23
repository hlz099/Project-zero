

/************************************************************/
/************************************************************/
/***********  miscellaneous Math Routines *******************/
/************************************************************/
/************************************************************/ 
/************************************************************/
/************************************************************/

/* Subroutine */ double binsrh_(xbrk, ybrk, nbrk, x)
     double xbrk[], ybrk[], *x;
     int *nbrk;
{

  /* Local variables */
  static int nbot, ntop, nhalf;
  static double y, slope, x1, x2, y1, y2, xs;

  /*          THIS IS A BINARY SEARCH ALGORITHM */
  /*          THE ALGORITHM LOCATES THE APPROPRIATE TABLE */
  /*          ELEMENTS FOR THE ABSCISSA (XBRK), AND USES LINEAR */
  /*          INTERPOLATION TO SOLVE FOR THE ORDINATE  Y. */


  /*     CHECK FOR OVERRANGE ON THE TABLE */

  /*      TABLE UNDERFLOW */

  /* Function Body */
  if (*x < xbrk[0]) {
    xs = xbrk[0];
  }  else {
    /*         TABLE OVERFLOW */
    if (*x > xbrk[*nbrk - 1]) {
      xs = xbrk[*nbrk - 1];
    } else {
      /*         WITHIN TABLE */
      xs = *x;
    }
  }

  /*      NOTE: TABLE MUST BE IN ASCENDING ORDER FOR THE */
  /*      ABSCISSA */

  /*      INITIALIZE */
  nbot = 0;
  ntop = (*nbrk - 1);

  /*      BEGIN SEARCH */

  /* find the upper and lower nodes */
  for (;(nbot+1 < ntop);)
    {
      nhalf = (ntop - nbot)/2 + nbot;
 
      if ((xbrk[nbot] <= xs) && (xs <= xbrk[ nhalf]))
	ntop = nhalf;
      else
	nbot = nhalf;
    }

  /*     LINEAR INTERPOLATE RESULTING CORNER POINTS */
  x1 = xbrk[nbot];
  x2 = xbrk[ntop];

  y1 = ybrk[nbot];
  y2 = ybrk[ntop];

  slope = (y2 - y1) / (x2 - x1);
  y = (xs - x1) * slope + y1;

  return y;
} /* binsrh_ */


