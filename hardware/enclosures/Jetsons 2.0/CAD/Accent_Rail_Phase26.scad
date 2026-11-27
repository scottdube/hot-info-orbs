// Phase 26 corrected: continuous solid from decorative rail to each mounting peg.
// Two pegs 10mm apart; select 3.2 or 3.4mm diameter.
$fn=80;
peg_d=3.2;
module rail(){
 union(){
  hull(){translate([-9,0,0]) sphere(d=4); translate([9,0,0]) sphere(d=4);}
  for(dx=[-5,5]){
   // Connected stand-off begins INSIDE the rail (rail radius 2mm)
   translate([dx,1.2,0]) rotate([-90,0,0]) cylinder(d=5,h=5.9);
   // Peg overlaps the stand-off for a mechanically continuous solid
   translate([dx,6.5,0]) rotate([-90,0,0]) cylinder(d=peg_d,h=6.5);
  }
 }
}
rail();
