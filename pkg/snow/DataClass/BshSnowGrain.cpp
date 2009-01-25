#include"BshSnowGrain.hpp"
#include<Wm3Quaternion.h>

// a voxel is 20.4 microns (2.04 � 10-5 meters)
// the sample was 10.4mm hight


void BshSnowGrain::registerAttributes()
{
	GeometricalModel::registerAttributes();
	REGISTER_ATTRIBUTE(center);
	REGISTER_ATTRIBUTE(c_axis);
	REGISTER_ATTRIBUTE(start);
	REGISTER_ATTRIBUTE(end);
	REGISTER_ATTRIBUTE(color);
	REGISTER_ATTRIBUTE(selection);
	REGISTER_ATTRIBUTE(layer_distance);
	REGISTER_ATTRIBUTE(gr_gr); // slices
}

BshSnowGrain::BshSnowGrain(const T_DATA& dat,Vector3r c_ax,int SELECTION,Vector3r col, Real one_voxel_in_meters_is) : GeometricalModel()
{
	createIndex();

	if(SELECTION!=0)
	{
		color=col;

		c_axis=c_ax;
		c_axis.Normalize();
		selection=SELECTION;
		std::cout << "creating grain " << SELECTION << " c_axis " << c_axis << "\n";

		Vector3r sum(0,0,0);
		float count=0;

		int FRAMES=dat.size();
		int sx=dat[0].size();
		int sy=dat[0][0].size();
		for(int i=0;i<FRAMES;++i)
		{
			for(int x=0;x<sx;++x)
				for(int y=0;y<sy;++y)
					if( dat[i][x][y]==SELECTION )
					{
						sum+=Vector3r((float)x,(float)y,(float)i);
						count+=1.0;
					}
		}
		center=sum/count;

		std::cout << "finding start/end\n";

		start=search_plane(dat,center,c_axis);
		end  =search_plane(dat,center,-1.0*c_axis);
		
		std::cout << "(start-end).length() " << ((start-end).Length()) << "\n";

		Quaternionr q;
		q.Align(Vector3r(0,0,1),c_axis);
//////////////////////////////////////////////////////////
int II=0;
float prevL=(start-end).SquaredLength();
float tmpL;
Vector3r moving_center(0,0,0),moving_sum(0,0,0);
count=0;
layer_distance=3.0;
for(Vector3r S=start; (tmpL=(S-end).SquaredLength())<=prevL ; S-=c_axis*layer_distance, ++II)
{
	slices.resize(II+1);
	float vectorY1=5.0;
	float vectorX1=0.0;
	for(float angle=0.0f ; angle <= (2.0f*3.14159) ; angle+=0.2f)
	{		
		float vectorX=(5.0*sin(angle));
		float vectorY=(5.0*cos(angle));		
		Vector3r tmp(vectorX1,vectorY1,0);
		tmp=search(dat,S+moving_center,q*tmp);
		slices[II].push_back(tmp-center);
		vectorY1=vectorY;
		vectorX1=vectorX;

		moving_sum+=tmp;
		count+=1.0;
	}
	prevL=tmpL;

	moving_center=moving_sum/count-S;
	moving_sum=Vector3r(0,0,0);
	count=0;

	std::cout << "slices " << II << " S-end " << (S-end).SquaredLength() << "\n";
}
//////////////////////////////////////////////////////////
// we're done. Now scale everything.
BOOST_FOREACH(std::vector<Vector3r>& g,slices)
	BOOST_FOREACH(Vector3r& v,g)
		v *= one_voxel_in_meters_is;

c_axis.Normalize();
start = (start-center)*one_voxel_in_meters_is;
end = (end-center)*one_voxel_in_meters_is;
center *= one_voxel_in_meters_is;


		
		std::cout << "grain created, center " << center << " start " << start << " end " << end << "\n";
	} 
	else
		std::cout << "nothing selected!\n";
};
	
Vector3r BshSnowGrain::search(const T_DATA& dat,Vector3r c,Vector3r dir)
{
	dir.Normalize();
	dir=dir*0.3;

	Vector3r the_farthest(c);

	int FRAMES=dat.size();
	int sx=dat[0].size();
	int sy=dat[0][0].size();
	for( Vector3r current(c); 
		   ((int)(current[0])<sx    ) && ((int)(current[0])>=0) 
		&& ((int)(current[1])<sy    ) && ((int)(current[1])>=0) 
		&& ((int)(current[2])<FRAMES) && ((int)(current[2])>=0) 
		; current+=dir )

			if( dat[(int)(current[2])][(int)(current[0])][(int)(current[1])]==selection )
				the_farthest=current;

	return the_farthest;
};

Vector3r BshSnowGrain::search_plane(const T_DATA& dat,Vector3r c,Vector3r dir)
{
	Vector3r result=search(dat,c,dir);
	
	dir.Normalize();
	
	Quaternionr q;
	q.Align(Vector3r(0,0,1),dir);

	dir=dir*0.3;
	bool anything_found=true;
		
	for( ; anything_found==true ; result+=dir)
	{
		anything_found=false;

		float vectorY1=5.0;
		float vectorX1=0.0;
		for(float angle=0.0f ; (angle <= (2.0f*3.14159)) && (anything_found==false) ; angle+=0.2f)
		{		
			float vectorX=(5.0*sin(angle));
			float vectorY=(5.0*cos(angle));		
			Vector3r tmp(vectorX1,vectorY1,0);
			tmp=search(dat,result,q*tmp);

			if((tmp-result).SquaredLength()>1.0)
				anything_found=true;
			else
				anything_found=false;

			vectorY1=vectorY;
			vectorX1=vectorX;			
		}

	}

	return result;
};

bool BshSnowGrain::is_point_orthogonally_projected_on_triangle(Vector3r& a,Vector3r& b,Vector3r c,Vector3r& N,Vector3r& P,Real point_plane_distance)
{
	// first check point - plane distance
	if(point_plane_distance == 0.0)
		point_plane_distance = N.Dot(P - c);
	if( point_plane_distance < 0 )            // point has to be inside - on negative side of the plane 
	{
		// now calculate projection of point in the plane
		Vector3r d(P - point_plane_distance*N);
		// now check if the point (when projected on a plane) is within triangle a,b,c
		// it could be faster with methods from http://softsurfer.com/Archive/algorithm_0105/algorithm_0105.htm
		// but I don't understand them, so I prefer to use the method which I derived myself
		Vector3r c1((a - b).Cross(d - a)); // since points a,b,c are all clockwise, 
		Vector3r c2((c - a).Cross(d - c)); // then if I put a point 'd' inside a triangle it will be clockwise
		Vector3r c3((b - c).Cross(d - b)); // with each pair of other points, but will be counterclockwise if it's outside
		if(c1.Dot(N) > 0 && c2.Dot(N) > 0 && c3.Dot(N) > 0) // therefore if any of them is counterclockwise, the dot product will be negative
			return true;
	}
	return false;
};
		
bool BshSnowGrain::is_point_inside_polyhedron(Vector3r P)
{
	const std::vector<boost::tuple<Vector3r,Vector3r,Vector3r,Vector3r> >& f(get_faces_const_ref());
	// loop on all faces
	size_t S(f.size());
	for(size_t i = 0; i < S ; ++i)
	{
		Vector3r a(get<0>(f[i]));
		Vector3r b(get<1>(f[i]));
		Vector3r c(get<2>(f[i]));
		Vector3r N(get<3>(f[i]));
		Real depth = m_depths[i];
		Real point_plane_distance = N.Dot(P - c);
		if(   point_plane_distance < 0             // point has to be inside - on negative side of the plane
		   && point_plane_distance > depth ) // and has to be within the depth of this face 
		{
			if(is_point_orthogonally_projected_on_triangle(a,b,c,N,P,point_plane_distance))
			{
				if( point_plane_distance > depth*0.5 ) // (1-parallelepiped)
				{ // that's close enough. We can speed up the computation by returning true at this point

// therefore in fact, we are checking if point is inside a volume of parallelepiped (1) the height of 1/2 depth
// within the polyhedron PLUS (that's the code below - the (2)) a polyhedron the height given by depth
// like this:
//              ____________     this weird shape is WHOLE polyhedron
//             /         Z. \                                                         ------
//            /          ' ` \                                                             |
//           /          '2-tetrahedron with the height equal to depth of THIS trangle      |
//          /          '     ` \                                                           | 
//         /          '       ` \                                                          |d
//        /          '         ` \______________ XX face (see description)                 |e - search below for
//       /          '           `              /                                           |p 'arbitrary safety coefficient'
//      /      ....'.............`......      /                                            |t
//      \     '   '           1-parallelepiped with height equal to 1/2 of depth           |h
//       \    '  '                 `   '    /                                              |
//        \   ' '                   `  '   /                                               |
//         \  ''                     ` '  /                                                |
//          \ '                       `' /                                                 |
//           \__________________________/                                             ------
//               THIS triangular face                
//
// depth of a triangle is the distance from it to the opposite side of polyhedron, kind of
// diameter in a sphere. The depth is calculated in four places: at three nodes of a triangle,
// and in its center point. The shallowest one is chosen as the actual value. This is
// not describing volume in exactly perfect math, but good enough, and rather fast.
// Sometimes for example the 'depth' of some triangle can extend the volume of the real polyhedron a bit
// on the other side. For example if that 'XX face' was closer to 'THIS triangular face' it would 
// still be considered as inside of polyhedron, because the '1/2 depth parallelepiped' might extend beyond it.
// this could happen if none of four (points a,b,c and center point) reached 'XX face' when calculating depth,
// because it was off a bit, and depth calculation ended up on another face.

// From this approach you can see that it's optimized for polyhedrons with large number of faces. For instance it will
// be terribly wrong if polyhedron is just a single four-noded tetrahedron. But it will work well if polyhedron
// is made from many triangles.
					return true;
				}
				else
				{ // (2-terhahdron)
					// so a point orthogonally projected on triangle is crossing it
					// now check if this point is inside a tetrahedron made by this triangle
					// and a point 'Z' at 'depth', see picture
					Vector3r Z((a+b+c)/3.0 + N*depth);
					Vector3r N1((Z - a).Cross(a - b)); // normals of each face
					Vector3r N2((Z - c).Cross(c - a)); // of tetrahedron a,b,c,Z
					Vector3r N3((Z - b).Cross(b - c));
					if(
						is_point_orthogonally_projected_on_triangle(b,a,Z,N1,P) &&
						is_point_orthogonally_projected_on_triangle(a,c,Z,N2,P) &&
						is_point_orthogonally_projected_on_triangle(c,b,Z,N3,P)
						)
						return true;
				}
			}
		}
	}
	return false;
};

bool BshSnowGrain::face_is_valid(Vector3r& a,Vector3r& b,Vector3r& c)
{
	if(a != b && b != c && c != a)
		return true;
	return false;
};

void BshSnowGrain::push_face(Vector3r a,Vector3r b,Vector3r c)
{
	if(face_is_valid(a,b,c))
	{
		Vector3r n((a - b).Cross(c - a));
		if(n.SquaredLength() != 0)
		{
			n /= n.Length();
			m_faces.push_back(boost::make_tuple(a,b,c,n));
		} else
		{
			std::cerr << "Face has no normal!\n";
			n=Vector3r(1,0,0);
		}
	}
}
		
int BshSnowGrain::how_many_faces()
{
	if(m_how_many_faces != -1)
		return m_how_many_faces;

// sorry, I never got around to find time and to check out how LOG_WARN works .... std::cerr is fine for me ;)
	std::cerr << "\nrecalculating the depths of polyhedron triangular faces - for faster collision detection\n";


// CREATE TRIANGULAR FACES. usually a polyhedron has triangular faces, but here it's a snow grain.
// it has layers, not faces, I have to make faces from layers

	m_faces.clear();
	//calculate amount of faces..

	// connected to START - the middle point in first layer
	int S = slices[0].size();
	Vector3r START(slices[0][0]);
	for(int j = 1 ; j < S ; ++j)
		START += slices[0][j];
	START /= (float)(S);
	for(int j = 0 ; j < S ; ++j)
		push_face( slices[0][j] , slices[0][(j+1 < S) ? (j+1):0] , START );

	// all triangles between layers
	int L = slices.size();
	for(int i = 1 ; i < L ; ++i)
		for(int j = 0 ; j < S/*slices[i].size()*/ ; ++j)
		{
			push_face( slices[i][j] , slices[i-1][j] , slices[i-1][(j-1 > 0) ? (j-1):(S-1)] );
			push_face( slices[i][j] , slices[i][(j+1 < S) ? (j+1):0] , slices[i-1][j] );
		}
	
	// connected to END - the middle point in last layer
	Vector3r END(slices[L-1][0]);
	for(int j = 1 ; j < S ; ++j)
		END += slices[L-1][j];
	END /= (float)(S);
	for(int j = 0 ; j < S ; ++j)
		push_face( slices[L-1][j] , END , slices[L-1][(j+1 < S) ? (j+1):0] );
	
	m_how_many_faces = m_faces.size();


// NOW I HAVE FACES. That's the code for usual polyhedrons, that already have faces:
// calculating the depth for each face.

	// now calculate the depth for each face
	m_depths.resize(m_faces.size(),0);
	// loop on all faces
	size_t SS(m_faces.size());
	for(size_t i = 0; i < SS ; ++i)
		m_depths[i] = calc_depth(i)*0.7; // 0.7 is an arbitrary safety coefficient

	return m_how_many_faces;
};
		
Real BshSnowGrain::calc_depth(size_t I)
{
	Vector3r A(get<0>(m_faces[I]));
	Vector3r B(get<1>(m_faces[I]));
	Vector3r C(get<2>(m_faces[I]));
	Vector3r N(get<3>(m_faces[I]));
	Vector3r P((A+B+C)/3.0);
	// (1) ray N is cast from point P, where P is on some triangle.
	// return the distance from P to next closest triangle

	Real depth = 0;
	const std::vector<boost::tuple<Vector3r,Vector3r,Vector3r,Vector3r> >& f(get_faces_const_ref());
	// loop on all faces
	size_t S(f.size());
	for(size_t i = 0; i < S ; ++i)
	{
		if(I != i) // don't check with itself
		{
			Vector3r n(get<3>(f[i]));
			Real parallel = n.Dot(N); // 'N' parallel to 'n' gives 0 dot product
			if( parallel < 0) // (2) must face in opposite directions
			{
				Vector3r a(get<0>(f[i]));
				Vector3r b(get<1>(f[i]));
				Vector3r c(get<2>(f[i]));
				for(int Z = 0 ; Z < 4 ; ++Z) // (ad. 1) OK, in fact it's not just from 'P' - we cast four rays.
				                             // From 'P' and all triangle nodes
				{
					Vector3r PP;
					switch(Z)
					{
						case 0 : PP = P; break;
						case 1 : PP = A; break;
						case 2 : PP = B; break;
						case 3 : PP = C; break;
					}
					Real neg_point_plane_distance = n.Dot(c - PP);
					if( neg_point_plane_distance > 0 ) // (ad. 2) must be facing towards each other
					{
						// now calculate intersection point 'd' of ray 'N' from point 'PP' with the plane
						Real u = neg_point_plane_distance/parallel;
						Vector3r d(PP + u*N);
						// now check if the point 'd' (when projected on a plane) is within triangle a,b,c
						Vector3r c1((a - b).Cross(d - a));
						Vector3r c2((c - a).Cross(d - c));
						Vector3r c3((b - c).Cross(d - b));
						if(c1.Dot(n) > 0 && c2.Dot(n) > 0 && c3.Dot(n) > 0)
						{
							if(depth == 0)
							{ 
								depth = u;
							} else {
								depth = std::max(depth , u ); // get the shallowest one
							}
						}
					}
				}
			}
		}
	}
	return depth;
}

YADE_PLUGIN("BshSnowGrain","Grrrr");

