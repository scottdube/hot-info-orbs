// Jetsons P20: forward-offset replacement feet. One STL; print two copies.
// Mount peg stays at x=0,y=0,z=0; enclosure underside at z=0.
// Device front is NEGATIVE Y. Wider contact extends forward.
$fn=64;
module foot(){
 union(){
  // Long continuous foot with stable flat bottom z=-20
  hull(){
   translate([0, -22, -18]) cube([19,29,4],center=true); // y -36.5 .. -7.5
   translate([0, 0, -18]) cube([16,13,4],center=true);  // y -6.5 .. +6.5
   translate([0, 0, -3]) cube([11,11,5],center=true);
  }
  // Peg from original Phase 22: Ø5.4, 7.4 straight + 0.6 taper
  translate([0,0,-1.0]) cylinder(d=5.4,h=8.4); // 1 mm overlap into body; fixes floating peg
  translate([0,0,7.4]) cylinder(d1=5.4,d2=4.8,h=0.6);
 }
}
foot();
