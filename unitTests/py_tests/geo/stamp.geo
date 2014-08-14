// GMSH geometry file for a thin stamp

E=1.0;  // stamp edge
T=0.1;  // stamp thickness

lf = 0.2;  // fine   meshing fineness
lc = 0.2;  // fine   meshing fineness

//////////////////////////////////////////////////
// upper surface
/////////////////////////////////////////////////
Point(100) = {       -E/2,   -E/2,    0, lf };
Point(101) = {       -E/2,    E/2,    0, lf };
Point(102) = {        E/2,    E/2,    0, lf };
Point(103) = {        E/2,   -E/2,    0, lf };

Line(100)  = { 100, 101 };
Line(101)  = { 101, 102 };
Line(102)  = { 102, 103 };
Line(103)  = { 103, 100 };

Line Loop(100) = {100, 101, 102, 103};
Plane Surface(100) = { 100 };

//////////////////////////////////////////////////
// sidewall and lower surface
/////////////////////////////////////////////////
Point(200) = {       -E/2,   -E/2,    -T, lc };
Point(201) = {       -E/2,    E/2,    -T, lc };
Point(202) = {        E/2,    E/2,    -T, lc };
Point(203) = {        E/2,   -E/2,    -T, lc };

// lower surface
Line(200)  = { 200, 201 };
Line(201)  = { 201, 202 };
Line(202)  = { 202, 203 };
Line(203)  = { 203, 200 };

Line Loop(200) = {200, 201, 202, 203};
Plane Surface(200) = { 200 };

// sidewall
Line(300)  = { 100, 200 };
Line(301)  = { 101, 201 };
Line(302)  = { 102, 202 };
Line(303)  = { 103, 203 };

// sidewall 1
// sign flips orientation
Line Loop(300) = {-100, 300, 200, -301};
Plane Surface(300) = { 300 };

// sidewall 2
Line Loop(301) = {-101, 301, 201, -302};
Plane Surface(301) = { 301 };

// sidewall 3
Line Loop(302) = {-102, 302, 202, -303};
Plane Surface(302) = { 302 };

// sidewall 3
Line Loop(303) = {-103, 303, 203, -300};
Plane Surface(303) = { 303 };


//////////////////////////////////////////////////
// list of all surfaces that we want to be meshed
//////////////////////////////////////////////////
Physical Surface(1) = { 100, 200, 300, 301, 302, 303 };

