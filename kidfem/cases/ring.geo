// Anneau (disque troué) : rayon intérieur a, extérieur b.
// Génère le .msh : gmsh -2 ring.geo -o ring.msh -format msh22
a  = 0.4;
b  = 1.0;
cl = 0.01;          // taille caractéristique des éléments

Point(1) = {0, 0, 0, cl};      // centre (sert de centre d'arc)
// Points d'axe sur le cercle extérieur (existent exactement -> épinglages)
Point(2) = { b, 0, 0, cl};
Point(3) = { 0, b, 0, cl};
Point(4) = {-b, 0, 0, cl};
Point(5) = { 0,-b, 0, cl};
// Points d'axe sur le cercle intérieur
Point(6) = { a, 0, 0, cl};
Point(7) = { 0, a, 0, cl};
Point(8) = {-a, 0, 0, cl};
Point(9) = { 0,-a, 0, cl};

Circle(1) = {2,1,3}; Circle(2) = {3,1,4}; Circle(3) = {4,1,5}; Circle(4) = {5,1,2}; // extérieur
Circle(5) = {6,1,7}; Circle(6) = {7,1,8}; Circle(7) = {8,1,9}; Circle(8) = {9,1,6}; // intérieur

Line Loop(1) = {1, 2, 3, 4};
Line Loop(2) = {5, 6, 7, 8};
Plane Surface(1) = {1, 2};     // domaine = extérieur moins le trou

Physical Line("outer")   = {1, 2, 3, 4};
Physical Line("inner")   = {5, 6, 7, 8};
Physical Surface("ring") = {1};

Mesh.MshFileVersion = 2.2;
