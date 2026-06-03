// Plaque rectangulaire L x H, maillée en triangles.
// Génère le .msh : gmsh -2 plate.geo -o plate.msh -format msh22
L  = 1.0;
H  = 0.2;
cl = 0.01;          // taille caractéristique des éléments

Point(1) = {0, 0, 0, cl};
Point(2) = {L, 0, 0, cl};
Point(3) = {L, H, 0, cl};
Point(4) = {0, H, 0, cl};

Line(1) = {1, 2};   // bas
Line(2) = {2, 3};   // droite
Line(3) = {3, 4};   // haut
Line(4) = {4, 1};   // gauche

Line Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

Physical Line("bottom")  = {1};
Physical Line("right")   = {2};
Physical Line("top")     = {3};
Physical Line("left")    = {4};
Physical Surface("plate") = {1};

Mesh.MshFileVersion = 2.2;
