// Phase 14 measured vertical alignment correction. Based on actual Phase 10 CAD.
// X right, Y toward rear, Z up. Notch geometry retained; Phase 12 horizontal positions retained; all five display apertures and notches lowered 2 mm relative to Phase 12.
// No decorative parts altered. PCB screw bosses = 7, 16mm tall.
// Rear-entry screws are a PROTOTYPE requiring actual fastener/PCB clearance validation.
// Jetsons Phase 8 accessory attachment CAD concept.
// World: X left/right, Y front/back (front = negative Y), Z up.
// PCB hole baseline and tray depth are retained; display centers provisional.
$fn=56;
part="front";
// Revision A: omit bottom-middle-left PCB boss and pilot hole (x=-19.5, z=-21.39).
// Phase 9: shell joining and locating features only. Display/button geometry unchanged.
fastener_x=[-70,70]; fastener_z=[-31,31];
W=270; H=76; D=30; wall=3; floor=3; corner=30;
front_w=270; front_h=76; front_r=30; display_shift=-3; button_rear_shift=5;
mount_x=[-107,-19.5,19.5,107]; mount_z=[-21.39,21.39];
display_x=[-95,-49,-3,43,89]; display_z=-2; display_d=35.6;
rear_relief_d=38.6; rear_relief_depth=1.5; connector_notch_w=24; connector_notch_top=28.5;
button_x=[-37,0,37];
module capsule(w,h,r){hull()for(x=[-w/2+r,w/2-r],z=[-h/2+r,h/2-r])translate([x,z])circle(r=r);}
// Extrude an XZ profile from rear to front: local 2D Y becomes world Z.
module profile(w,h,r,depth,front_y){translate([0,front_y+depth,0])rotate([90,0,0])linear_extrude(depth) capsule(w,h,r);}
module rear(){difference(){union(){difference(){profile(W,H,corner,D,-D/2);profile(W-2*wall,H-2*wall,corner-wall,D-floor+0.2,-D/2-0.1);}
// Screw towers extend toward front opening, fused into rear floor
for(x=fastener_x,z=fastener_z)translate([x,D/2-floor+0.4,z])rotate([90,0,0])cylinder(d=8,h=15.4);
// Bosses project forward from the interior rear wall; 16 mm to PCB support plane
for(x=mount_x,z=mount_z)if(!(x==-19.5 && z==-21.39))translate([x,D/2-floor+0.4,z])rotate([90,0,0])cylinder(d=8,h=16.4);
// Wide tapered integral base: reinforcement at the known break location.
for(x=mount_x,z=mount_z)if(!(x==-19.5 && z==-21.39))translate([x,D/2-floor+0.4,z])rotate([90,0,0])cylinder(d1=13,d2=8,h=6.4);
}
for(x=mount_x,z=mount_z)if(!(x==-19.5 && z==-21.39))translate([x,D/2-floor+0.1,z])rotate([90,0,0])cylinder(d=2.7,h=16.3);
// Four M2 pilot holes in rear shell screw towers; provisional placement.
for(x=fastener_x,z=fastener_z)translate([x,-D/2-0.2,z])rotate([-90,0,0])cylinder(d=3.1,h=D+0.5);
// Rear-access screw head counterbore through the outside back face.
for(x=fastener_x,z=fastener_z)translate([x,D/2-2.1,z])rotate([-90,0,0])cylinder(d=5.3,h=2.3);
// four front alignment pin receivers, placed beyond the PCB outline
for(x=[-30,30],z=[-31,31])translate([x,-D/2-0.2,z])rotate([-90,0,0])cylinder(d=3.6,h=5);
// fin keyed sockets: two vertical blind holes from upper surface
for(x=[-83,-59])translate([x,5,H/2-9])cylinder(d=5.4,h=12);
// antenna keyed socket at right top
translate([102,4,H/2-10])cylinder(d=6.4,h=13);
// foot sockets, vertical upward into underside (blind)
for(x=[-103,103])translate([x,0,-H/2-0.1])cylinder(d=5.4,h=10);
// top holes, normal to Z; not on the front face
for(x=button_x)translate([x,0,H/2-8])cylinder(d=6.5,h=12);
}}
module front(){difference(){union(){profile(front_w,front_h,front_r,3,-D/2-3);
// Blind screw receiving posts attached to the rear face of front plate.
for(x=fastener_x,z=fastener_z)translate([x,-D/2,z])rotate([-90,0,0])cylinder(d=7.5,h=8);
// Four alignment pins extend rearward into the shell
for(x=[-30,30],z=[-31,31])translate([x,-D/2-0.3,z])rotate([-90,0,0])cylinder(d=3,h=4.3); // front face
// shallow raised perimeter trim on front face
 difference(){profile(W-5,H-5,corner-2.5,1.5,-D/2-4.5);profile(W-14,H-14,corner-7,1.7,-D/2-4.6);}}
// Blind pilot holes in front posts: NO visible holes through the front face.
for(x=fastener_x,z=fastener_z)
 translate([x,-D/2-0.01,z])rotate([-90,0,0])cylinder(d=2.0,h=8.2);
// Revision D verified display geometry, transferred to XZ face.
// Through holes and merged connector notches pass along world Y.
for(x=display_x){
 translate([x,-D/2-5,display_z])rotate([-90,0,0])cylinder(d=display_d,h=9);
 // Back-side circular relief, 1.5 mm into rear of 3 mm front plate
 translate([x,-D/2-1.5,display_z])rotate([-90,0,0])cylinder(d=rear_relief_d,h=2);
 // Through-notch merges into the round aperture at Z=0; extends 28.5 mm above display center.
 translate([x-connector_notch_w/2,-D/2-5,display_z])cube([connector_notch_w,9,connector_notch_top]);
}
// Four rail units, two mounting pegs each, holes through front panel.
for(side=[-1,1], z=[-13,13], dx=[-5,5])
 translate([side*120+dx,-D/2-5,z])rotate([-90,0,0])cylinder(d=3.4,h=10);

}}
// Visual-only bezel placeholder. Its upper arc must be redesigned to
// avoid obscuring the verified connector notch; do not print yet.
module bezel(){difference(){cylinder(d=43,h=3.5);translate([0,0,-1])cylinder(d=35.6,h=6);}}
// 2D swept fin shape is XZ, 5 mm thick in Y.
module fin(){union(){
translate([-90,6,H/2-2])rotate([90,0,0])linear_extrude(5)polygon(points=[[0,0],[7,19],[22,37],[45,48],[72,51],[59,43],[42,28],[37,0]]);
// Fin's two vertical 5 mm pegs reach into blind sockets on upper shell
for(x=[-83,-59])translate([x,5,H/2-8])cylinder(d=5,h=10);
}}
module antenna(){translate([102,4,H/2])union(){
translate([0,0,-9])cylinder(d=6,h=10);
cylinder(d=7,h=3);translate([0,0,3])cylinder(d=4,h=28);
translate([0,0,34])sphere(d=8);
// Removed floating decorative rings from prototype: they were disconnected solids.
}}
// Local foot origin at enclosure underside, peg rises into matching socket.
module foot(){union(){
hull(){translate([0,0,-18])cube([16,12,4],center=true);translate([4,0,-2])cube([10,10,4],center=true);}
translate([0,0,0])cylinder(d=5,h=8);
}}
// Rail local X horizontal, Y is depth, front of shell at y=-18.
// Rail body y=-24, two rearward pegs reach the front face and engage holes.
module rail(){union(){
 hull(){translate([-9,0,0])sphere(d=4);translate([9,0,0])sphere(d=4);}
 for(dx=[-5,5]){
   // visible solid stand-offs: no more floating rails
   translate([dx,3.3,0])rotate([-90,0,0])cylinder(d=5,h=3.8);
   translate([dx,6.5,0])rotate([-90,0,0])cylinder(d=3,h=6.5);
 }
}}
module button(){union(){cylinder(d=11,h=3);translate([0,0,-8])cylinder(d=5.5,h=9);}}
module assembly(){
color("#11A6AA")rear(); color("#F4E7CA")front();
for(x=display_x){color("#1D252C")translate([x,-D/2-4.6,display_z])rotate([90,0,0])cylinder(d=35,h=0.8);color("#BEC6CB")translate([x,-D/2-4.7,display_z])rotate([90,0,0])bezel();}
for(x=button_x)color("#C4CDD0")translate([x,0,H/2+3])button();
color("#F4E7CA")fin(); color("#C6CED1")antenna();
for(x=[-103,103])color("#F4E7CA")translate([x,0,-H/2])foot();
for(x=[-120,120],z=[-13,13])color("#C7D0D3")translate([x,-D/2-9,z])rail();
}
// Three print-sized segments, boundaries lie between adjacent display centers.
// These are FIT/ALIGNMENT PROTOTYPES only: no structural seam joiners yet.
module segment(a,b){intersection(){front();translate([a,-40,-60])cube([b-a,80,120]);}}
if(part=="front_left")segment(-140,-69);
else if(part=="front_center")segment(-69,69);
else if(part=="front_right")segment(69,140);
else if(part=="rear")rear();else if(part=="front")front();else if(part=="bezel")bezel();else if(part=="fin")fin();else if(part=="antenna")antenna();else if(part=="foot")foot();else if(part=="button")button();else if(part=="rail")rail();else assembly();
