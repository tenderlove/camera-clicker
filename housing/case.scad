W = 28;
D = 95;
H = 25;

PCB_W = W;
PCB_D = D;
PCB_H = 1.6;

BATTERY_W = 22;
BATTERY_D = 70;
BATTERY_H = 21 - PCB_H;

PCB_Z = BATTERY_H;

USB_W = 10.5;
USB_D = 5;
USB_H = 3.5;

POWER_SWITCH_W = 5;
POWER_SWITCH_D = 4;
POWER_SWITCH_H = 3.5;

CORNER_D = 4.5;
CORNER_W = 6;
WALL = 1.5;

PUSH_BUTTON_H = 1.3;
PIN_DIAMETER = 1.1;
BOLT_DIAMETER = 2.2;

module NegativeSpace() {
  difference() {
    translate([W / 2, D / 2, 0]) {
      translate([0, 0, H / 2])
        difference() {
          cube([W, D, H], center=true);

          // Corner Stands
          color("green")
            for (i = [-1, 1]) {
              for (j = [-1, 1]) {
                w = W - CORNER_W;
                d = D - CORNER_D;
                translate([(i * w / 2), (j * d / 2), 0])
                  cube([CORNER_W, CORNER_D, H + 1], center=true);
              }
            }
        }

      translate([0, 0, PCB_Z + (PCB_H / 2)])
        cube([W, D, PCB_H], center=true);

      // Screw holes
      color("blue")
        for (i = [-1, 1]) {
          for (j = [-1, 1]) {
            w = 24;
            d = 91;
            translate([(i * w / 2), (j * d / 2), -(WALL + 1)])
              cylinder(H + (WALL * 2) + 2, d = BOLT_DIAMETER, $fn=90);
          }
        }
    }

    translate([PCB_W / 2, D - 13, PCB_Z + PCB_H + PUSH_BUTTON_H])
      cylinder(H - PCB_Z - PCB_H - PUSH_BUTTON_H, d = 3, $fn = 90);
  }

  translate([(PCB_W / 2) - (USB_W / 2), -USB_D, PCB_Z + PCB_H])
    cube([USB_W, USB_D, USB_H]);

  translate([PCB_W, 77.4, PCB_Z + PCB_H])
    cube([POWER_SWITCH_W, POWER_SWITCH_D, POWER_SWITCH_H]);

  // Pin hole
  translate([PCB_W / 2, D - 13, PCB_Z + PCB_H + PUSH_BUTTON_H])
    cylinder(5, d = PIN_DIAMETER, $fn = 90);
}

module Box() {
  difference() {
    w = WALL * 2;
    translate([-WALL, -WALL, -WALL])
      cube([W + w, D + w, H + w]);
    NegativeSpace();
  }
}

module Top() {
  difference() {
    Box();
    whall = WALL + 1;
    w = (whall) * 2;
    translate([-whall, -whall, -whall])
      cube([W + w, D + w, BATTERY_H + whall + PCB_H]);
  }
}

module Bottom() {
  intersection() {
    Box();
    whall = WALL + 1;
    w = (whall) * 2;
    translate([-whall, -whall, -whall])
      cube([W + w, D + w, BATTERY_H + whall + PCB_H]);
  }
}
Top();
//Bottom();
//NegativeSpace();
