/*
 * =====================================================================================
 *
 *       Filename:  xPointToCircle.cpp
 *
 *    Description:  constrain a point to a circle
 *
 *        Version:  1.0
 *        Created:  07/24/2015 01:24:49
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Pablo Colapinto (), gmail->wolftype
 *   Organization:  wolftype
 *
 * =====================================================================================
 */


#include <vsr/vsr_app.h>
#include <vsr/form/vsr_rigid.h>

using namespace vsr;
using namespace vsr::cga;

struct MyApp : App
{

  //Some Variables
  bool bReset = false;
  float amt = 0;


  Frame frame;
  /*-----------------------------------------------------------------------------
   *  Setup Variables
   *-----------------------------------------------------------------------------*/
  void setup ()
  {
    ///Add Variables to GUI
    gui (amt, "amt", -100, 100) (bReset, "bReset");

    objectController.attach (&frame);
  }


  /*-----------------------------------------------------------------------------
   *  Draw Routines 
   *-----------------------------------------------------------------------------*/
  void onDraw ()
  {

    auto circle = frame.cxy ();

    auto point = calcMouse3D ();

    auto constrainedPoint = Constrain::PointToCircle (point, circle);

    draw (circle);                     //draw circle in white
    draw (constrainedPoint, 1, 0, 0);  //draw constrained point in red

    //Done.  Now here we illustrate the geometry of How it works:
    // point coplanar with circle closest to input point
    auto coplanar = Flat::location (Round::carrier (circle), point);
    // line through coplanar point and circle center
    auto line = coplanar ^ Round::sur (circle) ^ Inf (1);

    draw (coplanar, 0, 1, 0);
    draw (line, 1, 1, 1);

    // then calculate meet of line and surround of circle...
  }
};


int main ()
{

  MyApp app;
  app.start ();

  return 0;
}
