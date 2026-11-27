// Phase 24: five connector notch covers. Based on physically tested Phase 23 3mm hood.
// Height increased 1.5 mm upward; 1.2 mm bottom closure added.
// Coordinates: X horizontal, Y vertical, Z projects from front panel.
$fn=80;
module cover(){
  difference(){
    hull(){
      for(x=[-13.6,13.6],y=[2,12.5])
        translate([x,y,0]) cylinder(r=2.4,h=4.4);
    }
    // Retain 3mm deep connector cavity, leave bottom 1.2mm as closing lip.
    // Open at upper end to avoid trapping connector, but bottom edge now closed.
    translate([-12.3,1.2,-0.1]) cube([24.6,16,3.1]);
  }
}
cover();
