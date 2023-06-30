W = 28;
D = 95;
H = 25;

PCB_W = W;
PCB_D = D;
PCB_H = 1.8;

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

CORNER_D = 5;
CORNER_W = 5;
WALL = 1.5;

PUSH_BUTTON_H = 1.3; // Increase this to make a larger gap above the button
PIN_DIAMETER = 1.1;
BOLT_DIAMETER = 2.3;

BOLT_HEAD_D = 4;
BOLT_HEAD_Z = 2.1;
NUT_D = 4.8;
NUT_Z = 3.1;

MOUNTING_HOLE_X = 24;
MOUNTING_HOLE_Y = 91;

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
        for (i = [-1, 1]) {
          for (j = [-1, 1]) {
            w = MOUNTING_HOLE_X;
            d = MOUNTING_HOLE_Y;

            translate([(i * w / 2), (j * d / 2), BOLT_HEAD_Z + H - 0.5])
              rotate(180, [1, 0, 0])
              CounterSink(BOLT_HEAD_D, BOLT_HEAD_Z, BOLT_DIAMETER, facets=90);

            translate([(i * w / 2), (j * d / 2), -WALL - 0.1])
            CounterSink(NUT_D, NUT_Z, BOLT_DIAMETER, facets=6);

            translate([(i * w / 2), (j * d / 2), -(WALL + 1)]) {
              color("blue")
                cylinder(H + (WALL * 2) + 2, d = BOLT_DIAMETER, $fn=90);
            }
          }
        }
    }

    translate([PCB_W / 2, D - 13, PCB_Z + PCB_H + PUSH_BUTTON_H])
      cylinder(H - PCB_Z - PCB_H - PUSH_BUTTON_H, d = 2, $fn = 90);
  }

  translate([(PCB_W / 2) - (USB_W / 2), -USB_D, PCB_Z + PCB_H])
    cube([USB_W, USB_D, USB_H]);

  translate([PCB_W, 77.4, PCB_Z + PCB_H])
    cube([POWER_SWITCH_W, POWER_SWITCH_D, POWER_SWITCH_H]);

  // Pin hole
  translate([W / 2, D - 18, H])
    Tab();
    //cylinder(5, d = PIN_DIAMETER, $fn = 90);
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
      cube([W + w, D + w, BATTERY_H + whall + PCB_H - 0.1]);
  }
}

LAYER_HEIGHT = 0.2;

module CounterSink(countersink_d, countersink_z, bolt_d, facets = 6, floating = true) {
  cylinder(countersink_z, d=countersink_d, $fn=facets);

  if (floating) {
    // First bridge
    translate([0, 0, countersink_z + (LAYER_HEIGHT / 2)])
      intersection() {
        color("green")
          cube([bolt_d, countersink_d, LAYER_HEIGHT], center=true);
      }

    color("blue")
    // Second bridge
    translate([0, 0, countersink_z + (LAYER_HEIGHT / 2) + LAYER_HEIGHT])
      cube([bolt_d, bolt_d, LAYER_HEIGHT], center=true);
  }
}

module Tab() {
  TAB_X = 10;
  TAB_Y = 20;
  TAB_Z = WALL + 0.2;
  TAB_GAP = 1;
  translate([-TAB_X / 2, -TAB_Y / 2, -0.1])
  difference() {
    cube([TAB_X, TAB_Y, TAB_Z]);
    translate([TAB_GAP, -0.1, -0.1])
    cube([TAB_X - (TAB_GAP * 2), TAB_Y - (TAB_GAP - 0.1), TAB_Z + 0.2]);
  }
}

rendering = "full";

if (rendering == "top") {
  rotate(180, [0, 1, 0])
    Top();
}

if (rendering == "bottom") {
  Bottom();
}

if (rendering == "full") {
  Bottom();
  rotate(180, [0, 1, 0])
    Top();
}
