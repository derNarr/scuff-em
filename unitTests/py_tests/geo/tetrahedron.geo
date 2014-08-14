// GMSH geometry file for a tetrahedron

H=0.2;  // tetrahedron height
E=2/1.7320508075688772*H;
// sqrt(3) \approx 1.7320508075688772

lf = 1.2;  // fine   meshing fineness
lc = 1.2;  // fine   meshing fineness

//////////////////////////////////////////////////
// upper surface
/////////////////////////////////////////////////
Point(100) = {       -E/2,   -H*1/3,    0, lf };
Point(101) = {        E/2,   -H*1/3,    0, lf };
Point(102) = {          0,    H*2/3,    0, lf };
Point(104) = {          0,        0,   -H, lc };

Line(100)  = { 100, 101 };
Line(101)  = { 101, 102 };
Line(102)  = { 102, 100 };

Line(200)  = { 100, 104 };
Line(201)  = { 101, 104 };
Line(202)  = { 102, 104 };

Line Loop(100) = {100, 101, 102};
Line Loop(101) = {100, 201, -200};
Line Loop(102) = {101, 202, -201};
Line Loop(103) = {102, 200, -202};

Plane Surface(100) = { 100 };
Plane Surface(101) = { 101 };
Plane Surface(102) = { 102 };
Plane Surface(103) = { 103 };

//////////////////////////////////////////////////
// list of all surfaces that we want to be meshed
//////////////////////////////////////////////////
Physical Surface(1) = { 100, 101, 102, 103 };

