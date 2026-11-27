// P24 prototype: guided button for P23 stationary silver snap-in bezel.
// Existing bezel bore measured ~10.2 mm; existing cap 10.5 mm, stem 5.7 mm.
// z=0 at bottom of cap; tip ends at -9mm, preserving original reach.
$fn=128;
cap_d=12.4;
cap_h=3;
guide_d=9.65; // ~0.55mm diametral clearance to measured bezel bore
guide_length=6.0;
groove_d=5.25;
groove_length=1.4;
tip_d=6.25;
tip_length=5.6;
union(){
 cylinder(d=cap_d,h=cap_h);
 translate([0,0,cap_h])cylinder(d1=cap_d-1,d2=cap_d-2,h=0.8);
 translate([0,0,-guide_length])cylinder(d=guide_d,h=guide_length+0.15);
 translate([0,0,-guide_length-groove_length])cylinder(d=groove_d,h=groove_length+0.15);
 translate([0,0,-13])cylinder(d=tip_d,h=tip_length+0.15);
}
