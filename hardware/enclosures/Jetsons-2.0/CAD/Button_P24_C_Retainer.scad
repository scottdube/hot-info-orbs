// P24 side-install C-retainer: slides into a narrow groove in new button.
// Does NOT need to be stretched over full button stem diameter.
$fn=128;
od=14.0;
id=5.55;
mouth=4.85; // narrow side opening retains 5.25mm groove
thickness=1.15;
difference(){
 cylinder(d=od,h=thickness);
 translate([0,0,-0.1])cylinder(d=id,h=thickness+0.2);
 translate([-od/2-0.2,-mouth/2,-0.1])cube([od/2+0.4,mouth,thickness+0.2]);
}
