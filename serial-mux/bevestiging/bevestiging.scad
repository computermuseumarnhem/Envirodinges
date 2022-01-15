$fn = 90;

function inch(n) = 25.4 * n;

PI_BOARD = [85, 56, 1.25];
PWR_BOARD = [inch(0.80), inch(1.70), inch(0.04)];
NANO_BOARD = [53, 36.5, inch(0.05)];

PI_HOLE_LOCATIONS =	[[3.5, 3.5], [61.5, 3.5], [3.5, 52.5], [61.5, 52.5]];
PWR_HOLE_LOCATIONS = [[inch(0.10), inch(0.25)], [inch(0.70), inch(1.45)]];
NANO_HOLE_LOCATIONS = [[26.5-inch(1.65/2), 36.5/2], [26.5+inch(1.65/2), 36.5/2]];

module chamfer(radius) {
    offset(r=radius) offset(r=-radius) children();
}

module plaat(size, radius, flat=false) {
    if (flat) {
        chamfer(radius) square([size.x, size.y]);
    } else {
        linear_extrude(size.z) plaat(size, radius, true);
    }
}

module center(dim) {
    //translate(-[dim.x, dim.y, 0]/2) 
        children();
}

module pi_board(h = 0, flat=false) {
    translate([0, 0, h]) center(PI_BOARD) plaat(PI_BOARD, 3.5, flat);
}
module pwr_board(h = 0, flat=false) {
    translate([0, 0, h]) center(PWR_BOARD) plaat(PWR_BOARD, 0, flat);
}
module nano_board(h = 0, flat=false) {
    translate([0, 0, h]) center(NANO_BOARD) plaat(NANO_BOARD, 0, flat);
}

module pi_hole_locations() {
    center(PI_BOARD) {
        for (hole = PI_HOLE_LOCATIONS) {
            translate(hole) children();
        }
    }
}
module pwr_hole_locations() {
    center(PWR_BOARD) {
        for (hole = PWR_HOLE_LOCATIONS) {
            translate(hole) children();
        }
    }
}
module nano_hole_locations() {
    center(NANO_BOARD) {
        for (hole = NANO_HOLE_LOCATIONS) {
            translate(hole) children();
        }
    }
}


module post() {
    difference() {
        cylinder(d1=6, d2=5, h=5);
        cylinder(d=1.5, h=5);
    }
}

base_left = -15;
base_top = -10;
base_width = 15 + PI_BOARD.y + 10 + PWR_BOARD.y + 15;
base_height = 10 + PI_BOARD.x + 10;

pi_left = 0;
pwr_left = pi_left + PI_BOARD.y + 10;
nano_left = pwr_left + PWR_BOARD.y/2 - (NANO_BOARD.y/2);

pi_top = 0;
pwr_top = 0;
nano_top = pi_top + PI_BOARD.x - NANO_BOARD.x;

translate([ pi_top, pi_left, 0]) {
    % pi_board(5);
    pi_hole_locations() post();
}
translate([ pwr_top,  pwr_left, 0]) {
    % pwr_board(5);
    pwr_hole_locations() post();
}
translate([ nano_top,  nano_left, 0]) {
    % nano_board(5);
    nano_hole_locations() post();
}

from_top = 20;
from_left = 7;
mounts = [
    [ base_top + from_top,                base_left + from_left              ],
    [ base_top + from_top,                base_left + base_width - from_left ],
    [ base_top + base_height - from_top,  base_left + from_left              ],
    [ base_top + base_height - from_top,  base_left + base_width - from_left ]
];

module mount(diff=false) {
    if (!diff) {
        cylinder(d=12, h=3);
    } else {
        cylinder(d=4, h=3);
        translate([0,0,1]) cylinder(d1=4, d2=8, h=2);
    }
}

module holes() {
    
    linear_extrude(2)
    chamfer(5) offset(r=-2)
    difference() {
        union() {
            translate([ pi_top, pi_left, 0]) {
                pi_board(flat=true);
            }
            translate([ pwr_top,  pwr_left, 0]) {
                pwr_board(flat=true);
            }
            translate([ nano_top,  nano_left, 0]) {
                nano_board(flat=true);
            }
        };
        for (m = mounts) {
            translate(m) circle(d=14);
        }
        translate([ pi_top, pi_left, 0]) {
            pi_hole_locations() circle(d=10);
        }
        translate([ pwr_top,  pwr_left, 0]) {
            pwr_hole_locations() circle(d=10);
        }
        translate([ nano_top,  nano_left, 0]) {
            nano_hole_locations() circle(d=10);
        }
    }
}


difference() {
    union() {
        translate([base_top, base_left, 0]) {
            plaat([base_height, base_width, 1], 5);
        }
        for (m = mounts) {
            translate(m) mount();
        }
    };
    for (m = mounts) {
        translate(m) mount(true);
    }
    holes();
}

module name(t) {
    linear_extrude(2)
        text(t, font="Quicksand:style=Bold", size=6, halign="center", valign="center");
}

text_center = [(mounts[0].x + mounts[2].x)/2, mounts[0].y];

translate(text_center) name("Envirodinges");