// GMSH geometry file for a infinitesimally thin plate

R=20.0;  // disc radius

lf = 0.5;  // fine   meshing fineness
lc = 5;  // coarse meshing fineness

//////////////////////////////////////////////////
//////////////////////////////////////////////////
/////////////////////////////////////////////////
Point(100) = {       0,   0,    0, lf };
Point(101) = {       R,   0,    0, lc };

Line(100)    = { 100, 101 };

Extrude{ {0, 0, 1}, {0, 0, 0}, 2*Pi/3 } { Line{100}; }
Extrude{ {0, 0, 1}, {0, 0, 0}, 2*Pi/3 } { Line{101}; }
Extrude{ {0, 0, 1}, {0, 0, 0}, 2*Pi/3 } { Line{104}; }

