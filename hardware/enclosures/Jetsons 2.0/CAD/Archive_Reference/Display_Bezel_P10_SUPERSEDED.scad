// Prototype 10: matching coupon and silver bezel. Units mm.
// Select part="coupon" or part="bezel".
part="coupon";
$fn=120;
aperture=37.0;
bezel_outer=45.0;
bezel_inner=37.4;
pin_circle=20.6;
pin_d=2.4;
pin_length=3.8;
hole_d=2.7;
angles=[0,180,270]; // pins avoid upper rectangular connector PCB
panel_thickness=3;
module pins(){for(a=angles) translate([pin_circle*cos(a),pin_circle*sin(a),0]) children();}
module coupon(){difference(){
 translate([-29.5,-32,0]) cube([59,66,panel_thickness]);
 translate([0,-6,-0.2]) cylinder(d=aperture,h=panel_thickness+0.4);
 // Rear connector relief retained from P09, aligned to display center y=-6.
 translate([-13,4,panel_thickness-1.2]) cube([26,18.5,1.4]);
 translate([0,-6,0]) pins() translate([0,0,-0.2]) cylinder(d=hole_d,h=panel_thickness+0.4);
}}
module bezel(){union(){
 difference(){cylinder(d=bezel_outer,h=1.4);translate([0,0,-0.1])cylinder(d=bezel_inner,h=1.6);}
 translate([0,0,1.4]) pins() cylinder(d=pin_d,h=pin_length);
}}
if(part=="coupon") coupon();
if(part=="bezel") bezel();
