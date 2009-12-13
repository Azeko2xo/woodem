#pragma once

#include "stdafx.h"
#include "def_types.h"
#include "CGAL/intersections.h"
#include "CGAL/squared_distance_3.h" 

namespace CGT {

// Types de sortie
typedef double Tenseur [9];


// Op�rations �l�mentaires :

Plan	PlanRadical	 (const Sphere& S1, const Sphere& S2);
Droite	Intersect (Plan &P1, Plan &P2);
Point	Intersect (Plan &P1, Plan &P2, Plan &P3);


// Op�rations sur les cellules


} //namespace CGT


