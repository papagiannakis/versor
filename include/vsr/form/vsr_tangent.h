/*
 * =====================================================================================
 *
 *       Filename:  vsr_tangent.h
 *
 *    Description:  tangent frames, contact methods, and coordinate surface six-spheres
                    in vsr::cga
 *
 *        Version:  1.0
 *        Created:  04/21/2016 16:18:57
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Pablo Colapinto (), gmail->wolftype
 *   Organization:  wolftype
 *
 * =====================================================================================
 */


#ifndef vsr_tangent_INC
#define vsr_tangent_INC

#include <vsr/form/vsr_cga3D_frame.h>

namespace vsr {
namespace cga {

#define FSIGN 1
#define FLIP false 

/// A much simpler rep
//  @todo determine how much this should just be operational and storage free
//  for instance are the DualSpheres needed?
struct TFrame
{

  TFrame () : tu (Tnv(1,0,0)), tv (Tnv(0,1,0)), tw(Tnv(0,0,1))
  {
    flatSurfaces();
  }
  TFrame (const Pair& _tu, const Pair &_tv, const Pair &_tw)
    : tu(_tu), tv(_tv), tw(_tw)
  {
    flatSurfaces();
  }

  // not negated unless tv are defined no^t and not t^no
   void flatSurfaces ()
   {
     svu = svw = Inf(FSIGN) <= tv;
     suv = suw = Inf(FSIGN) <= tu;
     swu = swv = Inf(FSIGN) <= tw;
   }

  // Tangents (orthonormal)
  Pair tu;
  Pair tv;
  Pair tw;
  // Spheres of constant p in direction q (spq)
  // @todo should these be set by the usurf functions below?
  // as is right now these default to planes
  // @Todo indexable?
  DualSphere svu;
  DualSphere swu;
  DualSphere suv;
  DualSphere swv;
  DualSphere suw;
  DualSphere svw;

  /// Surface of constant u with curvature ku (simplified)
  DualSphere usurf (float ku)
  {
    //return DualSphere (Inf (-1) <= tu).spin (Gen::bst (tu * -ku / 2.0));
    return Normalize( (Inf (FSIGN) <= tu) * ((tu * ku) + 1));
  }

  /// Surface of constant v with curvature kv
  DualSphere vsurf (float kv)
  {
    //return DualSphere (Inf (-1) <= tv).spin (Gen::bst (tv * -kv / 2.0));
    return Normalize( (Inf (FSIGN) <= tv) * ((tv * kv) + 1));
  }

  /// Surface of constant w with curvature kw
  DualSphere wsurf (float kw)
  {
    //return DualSphere (Inf (-1) <= tw).spin (Gen::bst (tw * -kw / 2.0));
    return Normalize ((Inf (FSIGN) <= tw) * ((tw * kw) + 1));
  }

  // Make the surfaces (needed to calculate rotor)
  void surfaces (float kvu, float kwu, float kuv, float kwv, float kuw,
                 float kvw)
  {
    svu = vsurf (kvu);
    swu = wsurf (kwu);
    suv = usurf (kuv);
    swv = wsurf (kwv);
    suw = usurf (kuw);
    svw = vsurf (kvw);
  }

  Vec du () { return -Round::dir (tu).copy<Vec> ().unit (); }
  Vec dv () { return -Round::dir (tv).copy<Vec> ().unit (); }
  Vec dw () { return -Round::dir (tw).copy<Vec> ().unit (); }

  Point pos () const { return Round::location (tu); }

  // Generate conformal transformation along u curve (fusion of kvu and kwu)
  Con uc (float kvu, float kwu, float dist)
  {
    return Gen::bst ((tv * kvu + tw * kwu) * -.5) * Gen::trs (du () * dist);
  }
  // Generate conformal transformation along v curve (fusion of kuv and kwv)
  Con vc (float kuv, float kwv, float dist)
  {
    return Gen::bst ((tu * kuv + tw * kwv) * -.5) * Gen::trs (dv () * dist);
  }
  // Generate conformal transformation along w curve (fusion of kuw and kvw)
  Con wc (float kuw, float kvw, float dist)
  {
    return Gen::bst ((tu * kuw + tv * kvw) * -.5) * Gen::trs (dw () * dist);
  }

  template <typename R>
  TFrame xf (const R &k, bool uflip, bool vflip, bool wflip)
  {
    Pair ttu = tu.spin (k);
    Pair ttv = tv.spin (k);
    Pair ttw = tw.spin (k);
    Pair ptu = Pair (-Round::dir (ttu).copy<Vec> ().unit ().copy<Tnv> ())
                 .trs (Round::location (ttu));
    Pair ptv = Pair (-Round::dir (ttv).copy<Vec> ().unit ().copy<Tnv> ())
                 .trs (Round::location (ttv));
    Pair ptw = Pair (-Round::dir (ttw).copy<Vec> ().unit ().copy<Tnv> ())
                 .trs (Round::location (ttw));
    return TFrame (ptu * (uflip ? -1 : 1), ptv * (vflip ? -1 : 1),
                   ptw * (wflip ? -1 : 1));
  }



  void uflip()
  {
    tu *= -1;
  }

  void vflip()
  {
    tv *= -1;
  }

  void wflip()
  {
    tw *= -1;
  }
 
  static DualSphere Normalize (const DualSphere&s ){
    float flip = (s[3] < 0) ? -1.0 : 1.0; 
    bool is_flat = (s[3] == 0);
    return ((is_flat) ? DualSphere(s[0], s[1], s[2], 0, s[4]) : s/s[3]) * flip;
  }
  // some extra stuff here -- the pair log generators bringing surf to surf
  static Pair CalcGen2 (const Point& p, const DualSphere& beg, const DualSphere&end)
  {
    auto ratio = (end/beg).tunit();
    bool isense = (p <= end)[0] > 0;
    bool iratio = ratio[0] < 0;;
    ratio *= iratio ? (isense ? -1.0 : 1.0) : 1.0;
//    ratio *= (isense ? -1.0 : 1.0);
    return Gen::log(ratio) / 2.0;
  }

  static Pair CalcGen (const Pair& p1, const Pair& p2, const DualSphere& beg, const DualSphere&end)
  {
    auto ratio = (end/beg).tunit();
    bool flipA = (beg <= p1)[3] > 0; 
    bool flipB = (end <= p2)[3] > 0; 
    bool flipC =  ((Round::location(p1) <= end)[0] > FPERROR) ;
    float flip = ((flipA != flipB) ) ? -1.0 : 1.0;      
    return Gen::log(ratio * flip, flipC, true) / 2.0;
    //return Gen::log(-ratio, flipC, true) / 2.0;
  }

  static Pair NormalizePair (const Pair &pair)
  {
    return Pair(-Round::dir (pair).copy<Vec> ().unit ().copy<Tnv> ())
                 .trs (Round::location (pair));
  };
  // note surfaces() must have been called
  // this sweeps the v direction curve "over" along u
  Pair duv (const TFrame &tf){
    return CalcGen(tu, tf.tu, suv, tf.suv);
  }
  // this sweeps the w direction curve "over" along u
  Pair duw (const TFrame &tf){
    return CalcGen(tu, tf.tu, suw, tf.suw);
  }
  // this sweeps the u direction curve "up" along v
  Pair dvu (const TFrame &tf){
    return CalcGen(tv, tf.tv, svu, tf.svu);
  }
  // this sweeps the w direction curve "up" along v
  Pair dvw (const TFrame &tf){
    return CalcGen(tv, tf.tv, svw, tf.svw);
  }
  // this sweeps the u direction curve "in" along w
  Pair dwu (const TFrame &tf){
    //return Gen::log ((tf.swu/swu).tunit()) / 2.0;
    return CalcGen(tw, tf.tw, swu, tf.swu);
  }
  // this sweeps the v direction curve "in" along w
  Pair dwv (const TFrame &tf){
    //return Gen::log ((tf.swv/swv).tunit()) / 2.0;
    return CalcGen(tw, tf.tw, swv, tf.swv);
  }

};


// Multiple TFrames, in a volume
struct TVolume {

  enum class Corner {
    ORIGIN = 0,
    U = 1,
    V = 2,
    W = 4,
    UV = 3,
    UW = 5,
    VW = 6,
    UVW = 7
  };

  enum class Face {
    LEFT,
    RIGHT,
    BOTTOM,
    TOP,
    BACK,
    FRONT
  };

  struct Mapping {

       std::vector<Con> mCon;
       int mResU, mResV, mResW;

       Mapping (int resU, int resV, int resW) : 
         mResU (resU), mResV (resV), mResW (resW) {
         mCon.resize(resU * resV * resW);
       }

       Con& at (int i, int j, int k){
         return mCon [i * mResV * mResW + j * mResW + k];
       }

       const Con& at (int i, int j, int k) const {
         return mCon [i * mResV * mResW + j * mResW + k];
       }

    };

    float mK[9];
    float mSpacing[3];
    TFrame mTFrame[8];
    Pair mGen[12];

    TVolume (){
      zeroInit();
    };

    TVolume (const TVolume& tv, Face face)
    {
      zeroInit();
      switch (face)
      {
        case Face::LEFT:
           {
             break;
           }
        case Face::RIGHT:
           {
             tf() = tv.uf();
             wf() = tv.uwf();
             vf() = tv.uvf();
             vwf() = tv.uvwf();

             calcSurfacesFromFace (Face::LEFT);

             break;
           }
         default:
           break;
      }
    };

    TVolume (float kvu, float kwu,
             float kuv, float kwv,
             float kuw, float kvw,
             float kv1u, float ku1w, float kw1v,
             float uSpacing, float vSpacing, float wSpacing)
    {
      mK[0] = kvu;
      mK[1] = kwu;
      mK[2] = kuv;
      mK[3] = kwv;
      mK[4] = kuw;
      mK[5] = kvw;
      mK[6] = kv1u;
      mK[7] = ku1w;
      mK[8] = kw1v;

      mSpacing[0] = uSpacing;
      mSpacing[1] = vSpacing;
      mSpacing[2] = wSpacing;

      calcSurfaces();
    };

    void zeroInit (){
      mK[0] = 0;
      mK[1] = 0;
      mK[2] = 0;
      mK[3] = 0;
      mK[4] = 0;
      mK[5] = 0;
      mK[6] = 0;
      mK[7] = 0;
      mK[8] = 0;

      mSpacing[0] = 3.0;
      mSpacing[1] = 3.0;
      mSpacing[2] = 3.0;

      calcSurfaces();
    }

    TFrame& tf(){ return mTFrame[0]; }
    TFrame& uf(){ return mTFrame[1]; }
    TFrame& vf(){ return mTFrame[2]; }
    TFrame& wf(){ return mTFrame[4]; }
    TFrame& uvf(){ return mTFrame[3]; }
    TFrame& uwf(){ return mTFrame[5]; }
    TFrame& vwf(){ return mTFrame[6]; }
    TFrame& uvwf(){ return mTFrame[7]; }

    const TFrame& tf() const { return mTFrame[0]; }
    const TFrame& uf() const { return mTFrame[1]; }
    const TFrame& vf() const { return mTFrame[2]; }
    const TFrame& wf() const { return mTFrame[4]; }
    const TFrame& uvf() const { return mTFrame[3]; }
    const TFrame& uwf() const { return mTFrame[5]; }
    const TFrame& vwf() const { return mTFrame[6]; }
    const TFrame& uvwf() const { return mTFrame[7]; }

    float& kvu(){ return mK[0]; }
    float& kwu(){ return mK[1]; }
    float& kuv(){ return mK[2]; }
    float& kwv(){ return mK[3]; }
    float& kuw(){ return mK[4]; }
    float& kvw(){ return mK[5]; }
    float& kv1u(){ return mK[6]; }
    float& ku1w(){ return mK[7]; }
    float& kw1v(){ return mK[8]; }

    const float& kvu() const { return mK[0]; }
    const float& kwu() const { return mK[1]; }
    const float& kuv() const { return mK[2]; }
    const float& kwv() const { return mK[3]; }
    const float& kuw() const { return mK[4]; }
    const float& kvw() const { return mK[5]; }
    const float& kv1u() const { return mK[6]; }
    const float& ku1w() const { return mK[7]; }
    const float& kw1v() const { return mK[8]; }

    float& uSpacing () { return mSpacing[0]; }
    float& vSpacing () { return mSpacing[1]; }
    float& wSpacing () { return mSpacing[2]; }

    const float& uSpacing () const { return mSpacing[0]; }
    const float& vSpacing () const { return mSpacing[1]; }
    const float& wSpacing () const { return mSpacing[2]; }

    Pair& duvw0() { return mGen[0]; }
    Pair& duwv0() { return mGen[1]; }

    Pair& dvwu0() { return mGen[2]; }
    Pair& dvuw0() { return mGen[3]; }

    Pair& dwuv0() { return mGen[4]; }
    Pair& dwvu0() { return mGen[5]; }

    Pair& duvw1() { return mGen[6]; }
    Pair& duwv1() { return mGen[7]; }

    Pair& dvwu1() { return mGen[8]; }
    Pair& dvuw1() { return mGen[9]; }

    Pair& dwuv1() { return mGen[10]; }
    Pair& dwvu1() { return mGen[11]; }

    const Pair& duvw0() const { return mGen[0]; }
    const Pair& duwv0() const { return mGen[1]; }

    const Pair& dvwu0() const { return mGen[2]; }
    const Pair& dvuw0() const { return mGen[3]; }

    const Pair& dwuv0() const { return mGen[4]; }
    const Pair& dwvu0() const { return mGen[5]; }

    const Pair& duvw1() const { return mGen[6]; }
    const Pair& duwv1() const { return mGen[7]; }

    const Pair& dvwu1() const { return mGen[8]; }
    const Pair& dvuw1() const { return mGen[9]; }

    const Pair& dwuv1() const { return mGen[10]; }
    const Pair& dwvu1() const { return mGen[11]; }

    //calculats based on four frames
    void calcSurfacesFromFace(const Face& face)
    {
      switch (face){
        case Face::LEFT:
          {
            tf().suv = TFrame::Normalize(vf().pos() <= tf().tu);
            tf().swv = TFrame::Normalize(vf().pos() <= tf().tw);
            tf().suw = TFrame::Normalize(wf().pos() <= tf().tu);
            tf().svw = TFrame::Normalize(wf().pos() <= tf().tv);
            vf().suw = TFrame::Normalize(wf().pos() <= vf().tu);
            vf().svw = TFrame::Normalize(wf().pos() <= vf().tv);
            wf().suv = TFrame::Normalize(vwf().pos() <= wf().tu);
            wf().swv = TFrame::Normalize(vwf().pos() <= wf().tw);

            kvu() = .3;
            kv1u() = -.3;
            // Conformal Rotors along u,v,w curves, passing in two curvatures
            Con uc = tf().uc (kvu(), kwu(), uSpacing());

            tf().svu = tf().vsurf (kvu());
            tf().swu = tf().wsurf (kwu());
            vf().svu = vf().vsurf (kv1u());
            uf().suw = uf().usurf (ku1w());

            uf() = tf().xf (uc, false, false, false);

            //ortho
            uf().suv = TFrame::Normalize(vf().svu <= uf().tu);
            uf().swv = TFrame::Normalize(vf().svu <= uf().tw);
            wf().swu = TFrame::Normalize(uf().suw <= wf().tw);
            wf().svu = TFrame::Normalize(uf().suw <= wf().tv);
            uf().svw = TFrame::Normalize(wf().swu <= uf().tv);
            vf().swu = TFrame::Normalize(uf().suv <= vf().tw);

            duvw0() = tf().duv (uf());
            duwv0() = tf().duw (uf());
            dvwu0() = tf().dvw (vf());
            dvuw0() = tf().dvu (vf());
            dwuv0() = tf().dwu (wf());
            dwvu0() = tf().dwv (wf());

            uvf() = vf().xf( Gen::boost(duvw0()), false, false, false);
            uwf() = uf().xf( Gen::boost(dwuv0()), false, false, false);

            //uvf() = vf().xf( Gen::boost(duvw0()), false, false, false);
            //uwf() = uf().xf( Gen::boost(dwuv0()), false, false, false);

            auto topPlane = vf().pos() ^ uvf().pos() ^ vwf().pos() ^ Inf(1);
            auto frontPlane = wf().pos() ^ vwf().pos() ^ uwf().pos() ^ Inf(1);
            auto rightPlane = uf().pos() ^ uwf().pos() ^ uvf().pos() ^ Inf(1);
            auto p = (topPlane.dual() ^ frontPlane.dual() ^ rightPlane.dual()).dual();
            p /= p[3];
            Point np = p.null();

            //these each have two defined surfaces
            uvf().suw = TFrame::Normalize(np <= uvf().tu);
            uvf().svw = TFrame::Normalize(np <= uvf().tv);
            vwf().svu = TFrame::Normalize(np <= vwf().tv);
            vwf().swu = TFrame::Normalize(np <= vwf().tw);
            uwf().swv = TFrame::Normalize(np <= uwf().tw);
            uwf().suv = TFrame::Normalize(np <= uwf().tu);

            //grab the tangent frame at np too
            //uvwf().tu = (np <= uvf().suw.undual()).dual();
            //uvwf().tv = (np <= uvf().svw.undual()).dual();
            //uvwf().tw = (np <= vwf().swu.undual()).dual();

            auto normT = [] (const Pair& pair){
               return Pair (-Round::dir (pair).copy<Vec> ().unit ().copy<Tnv> ());
            };
            uvwf().tu = normT(uvf().suw ^ np);
            uvwf().tv = normT(uvf().svw ^ np);
            uvwf().tw = normT(vwf().swu ^ np);
            //and with that, we have all the surfaces that can be defined with
            //nine coefficients -- 24 surfaces!  Which pair up into 12 Generators
            //now we find the remaining 6 generators
            // du of v direction at w=1 (sweeps left to right FRONT) |
            duvw1() = wf().duv(uwf());
            // du of w direction at v=1 (sweeps left to right TOP) /
            duwv1() = vf().duw(uvf());
            // dv of w direction at u=1 (sweeps bottom to top RIGHT) /
            dvwu1() = uf().dvw(uvf());
            // dv of u direction at w=1 (sweeps bottom to top FRONT) _
            dvuw1() = wf().dvu(vwf());
            // dw of u direction at v=1 (sweeps back to front TOP) _
            dwuv1() = vf().dwu(vwf());
            // dw of v direction at u=1 (sweeps back to front RIGHT) |
            dwvu1() = uf().dwv(uwf());
            break;
          }
          default:
            break;
        }
    }

    // caculates based on nine curvature coefficients and spacing
    void calcSurfaces ()
    {
      // Conformal Rotors along u,v,w curves, passing in two curvatures
      Con uc = tf().uc (kvu(), kwu(), uSpacing());
      Con vc = tf().vc (kuv(), kwv(), vSpacing());
      Con wc = tf().wc (kuw(), kvw(), wSpacing());

      //make the actual surfaces and store them
      tf().surfaces (kvu(), kwu(), kuv(), kwv(), kuw(), kvw());

      // New frames in respective directions, bools specify whether to "flip"
      uf() = tf().xf (uc, FLIP, false, false);
      vf() = tf().xf (vc, false, FLIP, false);
      wf() = tf().xf (wc, false, false, FLIP);

      // Here we can make three surfaces by bending
      // "Top Going Right"
      // "Right Going Forward"
      // "Front Going Up"
      vf().svu = vf().vsurf (kv1u());
      uf().suw = uf().usurf (ku1w());
      wf().swv = wf().wsurf (kw1v());

      //Now all other surfaces must be orthogonal to these
      //"Back Right Edge Going Up"
      //At u1, const u1 and const w0 in v dir are both ortho to const v1 in u dir
      //"Front Bottom Edge Going Over"
      //At w1, const w1 and const v0 in u dir are both ortho to const u1 in w dir
      //"Top Left Edge Going Forward"
      //At v1, const v1 and const u0 are both ortho to const w1 in v dir
      uf().suv = TFrame::Normalize(vf().svu <= uf().tu * FSIGN);
      uf().swv = TFrame::Normalize(vf().svu <= uf().tw * FSIGN);
      wf().swu = TFrame::Normalize(uf().suw <= wf().tw * FSIGN);
      wf().svu = TFrame::Normalize(uf().suw <= wf().tv * FSIGN);
      vf().svw = TFrame::Normalize(wf().swv <= vf().tv * FSIGN);
      vf().suw = TFrame::Normalize(wf().swv <= vf().tu * FSIGN);

      //now, what about the other surfaces to match the three bends?
      //"Bottom Going Forward"
      //"Back Going Right"
      //"Left Going Up"
      uf().svw = TFrame::Normalize(wf().swu <= uf().tv * FSIGN);
      vf().swu = TFrame::Normalize(uf().suv <= vf().tw * FSIGN);
      wf().suv = TFrame::Normalize(vf().svw <= wf().tu * FSIGN);

      // first letter indicates direction of sweep.
      // second letter indicates curvature line that is swept.
      // duv is a ratio of suvs, so sweeps right
      //
      // du of v direction (sweeps left to right BACK vertical) |
      duvw0() = tf().duv (uf());
      // du of w direction (sweeps left to right BOTTOM depth) /
      duwv0() = tf().duw (uf());
      // dv of w direction (sweeps bottom to top LEFT depth) /
      dvwu0() = tf().dvw (vf());
      // dv of u direction (sweeps bottom to top BACK horizontal) _
      dvuw0() = tf().dvu (vf());
      // dw of u direction (sweeps back to front BOTTOM horizontal) _
      dwuv0() = tf().dwu (wf());
      // dw of v direction (sweeps back to front LEFT veritical) |
      dwvu0() = tf().dwv (wf());

      // we need to calculate the other frames
      // to calculate the other surfaces
      uvf() = vf().xf( Gen::boost(duvw0()), FLIP, false, false);
      vwf() = wf().xf( Gen::boost(dvwu0()), false, FLIP, false);
      uwf() = uf().xf( Gen::boost(dwuv0()), false, false, FLIP);

      auto topPlane = vf().pos() ^ uvf().pos() ^ vwf().pos() ^ Inf(1);
      auto frontPlane = wf().pos() ^ vwf().pos() ^ uwf().pos() ^ Inf(1);
      auto rightPlane = uf().pos() ^ uwf().pos() ^ uvf().pos() ^ Inf(1);
      auto p = (topPlane.dual() ^ frontPlane.dual() ^ rightPlane.dual()).dual();
      p /= p[3];
      Point np = p.null();

      //these each have two defined surfaces
      uvf().suw = TFrame::Normalize(np <= uvf().tu * FSIGN);
      uvf().svw = TFrame::Normalize(np <= uvf().tv * FSIGN);
      vwf().svu = TFrame::Normalize(np <= vwf().tv * FSIGN);
      vwf().swu = TFrame::Normalize(np <= vwf().tw * FSIGN);
      uwf().swv = TFrame::Normalize(np <= uwf().tw * FSIGN);
      uwf().suv = TFrame::Normalize(np <= uwf().tu * FSIGN);

      //grab the tangent frame at np too
//      uvwf().tu = (np <= uvf().suw.undual()).dual();
//      uvwf().tv = (np <= uvf().svw.undual()).dual();
//      uvwf().tw = (np <= vwf().swu.undual()).dual();

      uvwf().tu = uvf().suw ^ np * FSIGN;
      uvwf().tv = uvf().svw ^ np * FSIGN;
      uvwf().tw = vwf().swu ^ np * FSIGN;
      //and with that, we have all the surfaces that can be defined with
      //nine coefficients -- 24 surfaces!  Which pair up into 12 Generators
      //now we find the remaining 6 generators
      // du of v direction at w=1 (sweeps left to right FRONT) |
      duvw1() = wf().duv(uwf());
      // du of w direction at v=1 (sweeps left to right TOP) /
      duwv1() = vf().duw(uvf());
      // dv of w direction at u=1 (sweeps bottom to top RIGHT) /
      dvwu1() = uf().dvw(uvf());
      // dv of u direction at w=1 (sweeps bottom to top FRONT) _
      dvuw1() = wf().dvu(vwf());
      // dw of u direction at v=1 (sweeps back to front TOP) _
      dwuv1() = vf().dwu(vwf());
      // dw of v direction at u=1 (sweeps back to front RIGHT) |
      dwvu1() = uf().dwv(uwf());

    }

   Con calcMappingAt (float ti, float tj, float tk){
      // Back to Front
      Boost wvu0 = Gen::bst (dwvu0() * tk);
      Boost wvu1 = Gen::bst (dwvu1() * tk);
      Boost wuv0 = Gen::bst (dwuv0() * tk);
      Boost wuv1 = Gen::bst (dwuv1() * tk);

      DualSphere su0v = TFrame::Normalize(tf().suv.spin (wvu0));
      DualSphere su1v = TFrame::Normalize(uf().suv.spin (wvu1));
      DualSphere sv0u = TFrame::Normalize(tf().svu.spin (wuv0));
      DualSphere sv1u = TFrame::Normalize(vf().svu.spin (wuv1));

      auto u0 = (tf().tu.spin (wvu0));
      auto u1 = (uf().tu.spin (wvu1));
      Pair duv = TFrame::CalcGen (u0, u1, su0v, su1v);
      auto v0 = (tf().tv.spin (wuv0));
      auto v1 = (vf().tv.spin (wuv1));
      Pair dvu = TFrame::CalcGen (v0, v1, sv0u, sv1u);
     
      Boost uv = Gen::bst(duv * ti);
      Boost vu = Gen::bst(dvu * tj);

      return vu * uv * wvu0;
   }

   Mapping calcMapping (int resU, int resV, int resW)
   {

     Mapping result (resU, resV, resW);

     for (int i = 0; i < resW; ++i)
     {
        float ti = (float)i/(resW-1);
        // Back to Front
        Boost wvu0 = Gen::bst (dwvu0() * ti);
        Boost wvu1 = Gen::bst (dwvu1() * ti);

        Boost wuv0 = Gen::bst (dwuv0() * ti);
        Boost wuv1 = Gen::bst (dwuv1() * ti);

        DualSphere su0v = TFrame::Normalize(tf().suv.spin (wvu0));
        DualSphere su1v = TFrame::Normalize(uf().suv.spin (wvu1));

        DualSphere sv0u = TFrame::Normalize(tf().svu.spin (wuv0));
        DualSphere sv1u = TFrame::Normalize(vf().svu.spin (wuv1));
       
        auto pmod = [](const Pair& pair){
           return Pair (-Round::dir (pair).copy<Vec> ().unit ().copy<Tnv> ())
                 .trs (Round::location (pair));
        }; 

        auto u0 = (tf().tu.spin (wvu0));
        auto u1 = (uf().tu.spin (wvu1));
        Pair duv = TFrame::CalcGen (u0, u1, su0v, su1v);

        auto v0 = (tf().tv.spin (wuv0));
        auto v1 = (vf().tv.spin (wuv1));
        Pair dvu = TFrame::CalcGen (v0, v1, sv0u, sv1u);
        
       for (int j = 0; j < resU; ++j)
       {
         float tj = (float)j/(resU-1);
         Boost uv = Gen::bst(duv * tj);

         for (int k = 0; k < resV; ++k)
         {
           float tk = (float)k/(resV-1);
           Boost vu = Gen::bst(dvu * tk);

           Con Kw = vu * uv * wvu0;
           result.at (j,k,i) = Kw;
         }
       }
     }

     return result;
   }

   struct Coord {
      VSR_PRECISION u,v,w;
   };
   Coord inverseMapping (const Point &p, const Face& face)
   {
     switch (face){
       case Face::LEFT:
         {
            auto sv  = p <= dvwu0();
            auto vpair  = Gen::log ( (sv/tf().svw).tunit()) * .5;
            auto tv = vpair <= !dvwu0();

            auto sw =  p <= dwvu0();
            auto wpair  = Gen::log ( (sw/tf().swv).tunit()) * .5;
            auto tw = wpair <= !dwvu0();
            return {0.0, tv[0], tw[0]};

           break;
         }
       case Face::RIGHT:
         {
            auto sv  = p <= dvwu1();
            auto vpair  = Gen::log ( (sv/uf().svw).tunit()) * .5;
            auto tv = vpair <= !dvwu1();

            auto sw =  p <= dwvu1();
            auto wpair  = Gen::log ( (sw/uf().swv).tunit()) * .5;
            auto tw = wpair <= !dwvu1();
            return {1.0, tv[0], tw[0]};

           break;
         }
       case Face::BACK:
         {
            auto sv  = p <= dvuw0();
            auto vpair  = Gen::log ( (sv/tf().svu).tunit()) * .5;
            auto tv = vpair <= !dvuw0();

            auto su =  p <= duvw0();
            auto upair  = Gen::log ( (su/tf().suv).tunit()) * .5;
            auto tu = upair <= !duvw0();
            return {tu[0], tv[0], 1.0};

           break;
         }
       case Face::FRONT:
         {
            auto tmp = [](const DualSphere& a, const DualSphere& b){
              return Ori(1) <= (Gen::log ((a/b).tunit()) * .5); 
            };

            auto tmp2 = [](const Pair& pa, const Pair& pb, const DualSphere& a, const DualSphere& b){
              return Ori(1) <= TFrame::CalcGen(pa, pb, a, b);; 
            };

            auto normT = [] (const Pair& pair){
               return pair;//Pair (-Round::dir (pair).copy<Vec> ().unit ().copy<Tnv> ());
            };

            auto sv = TFrame::Normalize(-p <= dvuw1());
            auto su = TFrame::Normalize(-p <= duvw1());

            auto svt = normT(sv ^ p);
            auto sut = normT(su ^ p);

            //auto vpair  = tmp(sv, wf().svu);
            //auto vpair2 = tmp(vwf().svu, wf().svu);
            //auto tv = vpair <= !vpair2;

            //auto upair = tmp (su, wf().suv);
            //auto upair2 = tmp (uwf().suv, wf().suv);
            //auto tu = upair <= !upair2;

            auto vpair  = tmp2(wf().tv, svt,  wf().svu, sv);
            auto vpair2 = tmp2(wf().tv, vwf().tv, wf().svu, vwf().svu);
            auto tv = vpair <= !vpair2;

            auto upair = tmp2 (wf().tu, sut, wf().suv, su);
            auto upair2 = tmp2 (wf().tu, uwf().tu, wf().suv, uwf().suv);
            auto tu = upair <= !upair2;

            float fv = FERROR(tv[0]) ? 0 : tv[0];
            float fu = FERROR(tu[0]) ? 0 : tu[0];

            return {fu, fv, 1.0};

            break;
         }
       default:
            return {0.0, 0.0, 0.0};

     }

   }

};

/// 3D Frame of Tangent Vectors (Point Pairs)
///
///  @todo make a six-sphere system vsr_coord
///  @todo currently inherits from Frame --
///  might want to be able to set parent class Frame::pos and Frame::rot
///  from tangent info stored here.
///  @todo possibly make more method-based, and less state-based
///  @todo how many spheres about a circle?  any number?
struct TangentFrame : public Frame
{
  /// Coordinate Surface Spheres (Sigma)
  ///  Sphere through next point with tangents as below (needed or put in next level?)
  Sphere sphere[3];
  /// kappa Tangent Vectors (null point pairs)
  Pair tan[3];
  /// Kappa  Tangent Bivectors (null circles)
  Circle bitan[3];

  /// @todo Derivatives in every possible direction
  //float curvature[6];

  /// Default Construct -- Store Frame as Tangents
  TangentFrame () : Frame () { store (); }

  /// Copy Construct from Frame -- Store Frame as Tangents
  TangentFrame (const Frame &f) : Frame (f) { store (); }

  /// Construct at position p w.r.t TangentFrame
  TangentFrame (const Point &p, const TangentFrame tf) { set (p, tf); }

  /// Assignment to Frame
  TangentFrame &operator= (const Frame &f)
  {
    this->rot () = f.rot ();
    this->pos () = f.pos ();
    return store ();
  }

  /// Assignment to TangentFrame (needed?)
  TangentFrame &operator= (const TangentFrame &f)
  {
    this->rot () = f.rot ();
    this->pos () = f.pos ();

    for (int j = 0; j < 3; ++j)
      {
        tan[j] = f.tan[j];
        bitan[j] = f.bitan[j];  // swap duality here
        sphere[j] = f.sphere[j];
      }

    return *this;
  }


  /// Save current Frame State
  TangentFrame &store ()
  {

    tan[0] = tx ();
    tan[1] = ty ();
    tan[2] = tz ();

    for (int j = 0; j < 3; ++j)
      {
        bitan[j] = tan[j].undual ();  // swap duality here
      }

    return *this;
  }

  /// Set current tangents at position from relative TangentFrame
  void set (const Point &p, const TangentFrame &tf)
  {
    this->pos () = p;
    // hmm, could potentially work in any dimension
    for (int j = 0; j < 3; ++j)
      {
        //this outer product order really matters for symmetry
        sphere[j] = tf.bitan[j] ^ p;
        bitan[j] = Tangent::at (sphere[j], p);
        tan[j] = bitan[j].dual ();
      }
  }

  /// Flip direction of ith Tangent (useful when chaining systems)
  TangentFrame &flip (int idx)
  {
    tan[idx] *= -1;
    bitan[idx] *= -1;
    return *this;
  }

  /// Make Unit Length (unneeded?)
  TangentFrame unit () const
  {
    TangentFrame tf = *this;
    for (int idx = 0; idx < 3; ++idx)
      {
        // normalize euclidean representation and reformulate
        // need to normalize coordinate surface?
        tf.sphere[idx] = sphere[idx] / (Round::direction (sphere[idx])[0]);
        tf.bitan[idx] =
          Circle (
            Round::direction (bitan[idx]).copy<Biv> ().tunit ().copy<Tnb> ())
            .translate (pos ());
        tf.tan[idx] = tf.bitan[idx].dual ();
      }
    return tf;
  }


  /// Calculate Edges as intersections of constant coordinates (legacy)
  Circle calcCurve (int idx)
  {
    Circle c;
    switch (idx)
      {
        case 0:
          c = (sphere[1].dual () ^ sphere[2].dual ()).undual ();
        case 1:
          c = (sphere[0].dual () ^ sphere[2].dual ()).undual ();
        case 2:
          c = (sphere[0].dual () ^ sphere[1].dual ()).undual ();
      }
    return c;
  }

  /// Get x DirectionVector
  Drv xdir () { return -Round::direction (tan[0]); }
  /// Get y DirectionVector
  Drv ydir () { return -Round::direction (tan[1]); }
  /// Get z DirectionVector
  Drv zdir () { return -Round::direction (tan[2]); }

  ///@todo rename this to xcop, ycop, zcop (for Curvature OPerator)
  //to disambiguate from xcurve
  /// Generate Boost Relative to x Tangent Vector
  Bst xcurve (float amt) { return Gen::bst (tan[0] * amt * -.5); }
  /// Generate Boost Relative to y Tangent Vector
  Bst ycurve (float amt) { return Gen::bst (tan[1] * amt * -.5); }
  /// Generate Boost Relative to z Tangent Vector
  Bst zcurve (float amt) { return Gen::bst (tan[2] * amt * -.5); }

  ///@todo rename these to xycop, yzcop, xzcop (for Curvature OPerator)
  /// Generate Boost Relative to const x and const y Tangent Vector
  Bst xycurve (float amtX, float amtY)
  {
    return Gen::bst ((tan[0] * amtX + tan[1] * amtY) * -.5);
  }

  /// Generate Boost Relative to x and z TangentVector (null Pair) (dx/dy + dz/dy)
  Bst xzcurve (float amtX, float amtZ)
  {
    return Gen::bst ((tan[0] * amtX + tan[2] * amtZ) * -.5);
  }
  /// Generate Boost Relative to y and z TangentVector (null Pair)
  Bst yzcurve (float amtY, float amtZ)
  {
    return Gen::bst ((tan[1] * amtY + tan[2] * amtZ) * -.5);
  }

  /// x = Constant Coordinate Surface from Boost generator
  DualSphere xsurface (const Bst &b)
  {
    return DualSphere (Round::carrier (bitan[0]).dual ()).spin (b);
  }
  /// y = Constant Coordinate Surface from Boost generator
  DualSphere ysurface (const Bst &b)
  {
    return DualSphere (Round::carrier (bitan[1]).dual ()).spin (b);
  }
  /// z = Constant Coordinate Surface from Boost generator
  DualSphere zsurface (const Bst &b)
  {
    return DualSphere (Round::carrier (bitan[2]).dual ()).spin (b);
  }

  /// An x = Constant Coordinate Surface from curvature scalar
  DualSphere xsurface (float amt)
  {
    return DualSphere (Round::carrier (bitan[0]).dual ().unit ())
      .spin (xcurve (amt));
  }

  /// A y = Constant Coordinate Surface from curvature scalar
  DualSphere ysurface (float amt)
  {
    return DualSphere (Round::carrier (bitan[1]).dual ().unit ())
      .spin (ycurve (amt));
  }

  /// A z = Constant Coordinate Surface from curvature scalar
  DualSphere zsurface (float amt)
  {
    return DualSphere (Round::carrier (bitan[2]).dual ().unit ())
      .spin (zcurve (amt));
  }

  /// Get position of TangentFrame
  Point point () const { return pos (); }

  /// Point along x direction
  Point xpoint (float amt) { return point ().translate (xdir () * amt); }
  /// Point along y direction
  Point ypoint (float amt) { return point ().translate (ydir () * amt); }
  /// Point along z direction
  Point zpoint (float amt) { return point ().translate (zdir () * amt); }

  /// TangentFrame yzcurved
  TangentFrame xbend (float amtY, float amtZ, float dist = 1)
  {
    auto bst = yzcurve (amtY, amtZ);
    auto pt = Round::location (xpoint (dist).spin (bst));
    return TangentFrame (pt, *this);
  }

  /// TangentFrame xzcurved
  TangentFrame ybend (float amtX, float amtZ, float dist = 1)
  {
    auto bst = xzcurve (amtX, amtZ);
    auto pt = Round::location (ypoint (dist).spin (bst));
    return TangentFrame (pt, *this);
  }

  /// TangentFrame xycurved
  TangentFrame zbend (float amtX, float amtY, float dist = 1)
  {
    auto bst = xycurve (amtX, amtY);
    auto pt = Round::location (zpoint (dist).spin (bst));
    return TangentFrame (pt, *this);
  }

  /// Use Const X Surface to Find fourth TangentFrame on a circle
  TangentFrame xclose (float amt, const Point &pa, const Point &pb)
  {
    return circleClose (xsurface (amt), pa, pb);
  }
  /// Use Const Y Surface to Find fourth TangentFrame on a circle
  TangentFrame yclose (float amt, const Point &pa, const Point &pb)
  {
    return circleClose (ysurface (amt), pa, pb);
  }
  /// Use Const Z Surface to Find fourth TangentFrame on a circle
  TangentFrame zclose (float amt, const Point &pa, const Point &pb)
  {
    return circleClose (zsurface (amt), pa, pb);
  }

  /// Given four tangent frames, find corner
  TangentFrame close (const TangentFrame &ta, const TangentFrame &tb,
                      const TangentFrame &tc, const TangentFrame &td)
  {

    auto pt = point ();

    auto cir = pt ^ ta.point () ^ tb.point ();
    auto cir2 = pt ^ tc.point () ^ td.point ();

    auto pair = (Round::surround (cir) ^ cir2.dual ()).dual ();

    auto tpa = Construct::pointA (pair);
    auto tpb = Construct::pointB (pair);

    auto p1 = FERROR ((pt <= tpa).wt ()) ? tpb : tpa;

    return TangentFrame (p1, *this);
  }

  /// Given a sphere and two points, make a circle with the two points and find its intersection with sphere
  TangentFrame circleClose (const DualSphere &s, const Point &pa,
                            const Point &pb)
  {

    auto pt = point ();
    auto cir = pt ^ pa ^ pb;
    auto pair = (s ^ cir.dual ()).dual ();

    /// @todo speed up this extraction
    auto tpa = Construct::pointA (pair);
    auto tpb = Construct::pointB (pair);

    auto p1 = FERROR ((pt <= tpa).wt ()) ? tpb : tpa;

    return TangentFrame (p1, *this);
  }
};

/**
* @brief Contact provides methods for calculations with a normal tangent vector at a point on a sphere

  contains a DualSphere, a Point position on the sphere,
  and a null Pair tangent vector specifying the normal on
  the sphere at that point.  Normal may be pointing inwards.
*/
struct Contact
{

  DualSphere sphere;
  Point point;
  Pair tnv;

  /// Construct from Point and DualPlane
  Contact (const Point &p, const DualPlane &dlp)
      : point (p), sphere (dlp), tnv ((p <= dlp.dual ()).dual ())
  {
  }

  /// Construct from Point and DualSphere
  Contact (const Point &p, const DualSphere &s)
      : point (p), sphere (s), tnv (Tangent::at (s.undual (), p).dual ())
  {
  }

  /// Construct from Point on some other source sphere and some target sphere
  /// (makes orthogonal plunge into target sphere)
  Contact (const Point &p, const DualSphere &source, const DualSphere &target)
      : sphere (target)
  {
    auto tnvsource = Tangent::at (source.undual (), p).dual ();  // A Point Pair
    point = Construct::pointA (((target ^ tnvsource).dual () ^ target).dual ());
    tnv = Tangent::at (target.undual (), point).dual ();
  }

  /// Construct from another contact and a target
  Contact (const Contact &source, const DualSphere &target) : sphere (target)
  {
    point =
      Construct::pointA (((target ^ source.tnv).dual () ^ target).dual ());
    tnv = Tangent::at (target.undual (), point).dual ();
  }

  /// Get undualized tangentbivector (circle)
  Cir bitan () const { return tnv.dual (); }  //*** dualization changed ***/
  /// Get Vector direction of TangentVector
  Vec vec () const { return -Round::direction (tnv).copy<Vec> ().unit (); }
  /// Get Bivector dual of vector direction
  Biv biv () const { return vec ().dual (); }  //*** dualization changed ***/
};


/// SixSphere Coordinate System curvatures in every direction (see Moon and Spencer for coordinate system diagram)
/// @todo simplify this representation -- tan and bitan should be methods?
struct SixSphere
{

  TangentFrame frame;
  struct Curve
  {
    float a = 0;
    float b = 0;
  } curve[3];  ///< Three Coordinate Curves, each with two curvatures

  float lengthX, lengthY, lengthZ;

  SixSphere () {}
  SixSphere (const Frame &f) : frame (f) {}
  //  SixSphere& operator= (const Frame& f){
  //    this->pos() = f.pos();  this->rot() = f.rot();
  //    store();
  //    return *this;
  //  }

  /// Set curvatures in every direction
  void set (float YX, float ZX,  // constant X in the Y and Z directions
            float XY, float ZY,  // constant Y in the X and Z directions
            float XZ, float YZ,  // constant Z in the X and Y directions
            float lx = 1, float ly = 1,
            float lz = 1)  //distances in each direction
  {
    curve[0].a = YX;
    curve[0].b = ZX;
    curve[1].a = XY;
    curve[1].b = ZY;
    curve[2].a = XZ;
    curve[2].b = YZ;

    lengthX = lx;
    lengthY = ly;
    lengthZ = lz;
  }

  /// Tangent Frame in x direction
  TangentFrame x ()
  {
    return frame.xbend (curve[0].a, curve[0].b, lengthX).unit ();
  }
  /// Tangent Frame in y direction
  TangentFrame y ()
  {
    return frame.ybend (curve[1].a, curve[1].b, lengthY).unit ();
  }
  /// Tangent Frame in z direction
  TangentFrame z ()
  {
    return frame.zbend (curve[2].a, curve[2].b, lengthZ).unit ();
  }
  /// Tangent Frame in x+y direction (with optional additional curve input)
  TangentFrame xy (float c = 0.f)
  {
    float XY = cxy ();
    float X1Y = XY == 0 ? 0 : XY > 0 ? -(1.f / ((1.f / XY) - lengthX))
                                     : 1.f / ((1.f / -XY) + lengthX);
    return x ().xclose (X1Y + c, frame.pos (), y ().pos ()).unit ();
  }
  /// Tangent Frame in x+z direction (with optional additional curve input)
  TangentFrame zx (float c = 0.f)
  {
    float ZX = czx ();
    float Z1X = ZX == 0 ? 0 : ZX > 0 ? -(1.f / ((1.f / ZX) - lengthZ))
                                     : 1.f / ((1.f / -ZX) + lengthZ);
    return z ().zclose (Z1X + c, frame.pos (), x ().pos ()).unit ();
  }
  /// Tangent Frame in y+z direction (with optional additional curve input)
  TangentFrame zy (float c = 0.f)
  {
    float ZY = czy ();
    float Z1Y = ZY == 0 ? 0 : ZY > 0 ? -(1.f / ((1.f / ZY) - lengthZ))
                                     : 1.f / ((1.f / -ZY) + lengthZ);
    return z ().zclose (Z1Y + c, frame.pos (), y ().pos ()).unit ();
  }
  /// Tangent Frame in x+y+z direction
  TangentFrame xyz (float cx = 0.f, float cy = 0.f)
  {
    return xy ().close (x (), zx (cx), y (), zy (cy)).unit ();
  }

  float &cyx () { return curve[0].a; }
  float &czx () { return curve[0].b; }
  float &cxy () { return curve[1].a; }
  float &czy () { return curve[1].b; }
  float &cxz () { return curve[2].a; }
  float &cyz () { return curve[2].b; }

  //TangentFrame xy(float amtX1Y){ return x().xclose( amtX1Y, pos(), y().pos() ); }
  //TangentFrame xyz(){ return }
};
}
}  //vsr::cga::

#endif /* ----- #ifndef vsr_tangent_INC  ----- */
