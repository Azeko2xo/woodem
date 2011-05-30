// Voronoi calculation example code
//
// Author   : Chris H. Rycroft (LBL / UC Berkeley)
// Email    : chr@alum.mit.edu
// Date     : July 1st 2008

#include "voro++.cc"
#include "r_table.cc"

// A guess for the memory allocation per region
const int memory=8;

// A maximum allowed number of regions, to prevent enormous amounts of memory
// being allocated
const int max_regions=16777216;

// A buffer size
const int bsize=2048;

// Output routine
template<class c_option>
void output(c_option &con,char *buffer,int bp);
template<class c_option>
void output(c_option &con,char *buffer,int bp);

int main(int argc,char **argv) {

	char *farg,buffer[bsize];
	bool radial;int i,n,bp;
	fpoint bx,bxy,by,bxz,byz,bz,x,y,z;

	// Check the command line syntax
	if(argc==2) {
		radial=false;farg=argv[1];
	} else if(argc==3&&strcmp(argv[1],"-r")==0) {
		radial=true;farg=argv[2];
	} else {
		cerr << "Syntax: ./network [-r] <filename.v1>" << endl;
		return VOROPP_CMD_LINE_ERROR;
	}

	// Check that the file has a ".v1" extension
	bp=strlen(farg);
	if(bp+2>bsize) {
		cerr << "Filename too long" << endl;
		return VOROPP_CMD_LINE_ERROR;
	}
	if(bp<3||farg[bp-3]!='.'||farg[bp-2]!='v'||farg[bp-1]!='1') {
		cerr << "Filename must end in '.v1'" << endl;
		return VOROPP_CMD_LINE_ERROR;
	}

	// Try opening the file
	ifstream is;
	is.open(farg,ifstream::in);
	if(is.fail()) voropp_fatal_error("Unable to open file for import",VOROPP_FILE_ERROR);

	// Read header line
	is.getline(buffer,bsize);
	cout << buffer << endl;
	if(strcmp(buffer,"Unit cell vectors:")!=0)
		voropp_fatal_error("Invalid header line",VOROPP_FILE_ERROR);
	
	// Read in the box dimensions
	is.width(bsize);
	is >> buffer;
	if(strcmp(buffer,"va=")!=0) voropp_fatal_error("Invalid first vector",VOROPP_FILE_ERROR);
	is >> bx >> x >> x;
	is >> buffer;
	if(strcmp(buffer,"vb=")!=0) voropp_fatal_error("Invalid second vector",VOROPP_FILE_ERROR);
	is >> bxy >> by >> x;	
	is >> buffer;
	if(strcmp(buffer,"vc=")!=0) voropp_fatal_error("Invalid third vector",VOROPP_FILE_ERROR);
	is >> bxz >> byz >> bz;
	
	is >> n;

	// Print the box dimensions
	printf("Box dimensions:\n");
	printf("  va=(%f 0 0)\n",bx);
	printf("  vb=(%f %f 0)\n",bxy,by);
	printf("  vc=(%f %f %f)\n\n",bxz,byz,bz);

	// Check that the input parameters make sense
	if(n<1) voropp_fatal_error("Invalid number of particles",VOROPP_FILE_ERROR);
	if(bx<tolerance||by<tolerance||bz<tolerance)
		voropp_fatal_error("Invalid box dimensions",VOROPP_FILE_ERROR);

	// Compute the internal grid size, aiming to make
	// the grid blocks square with around 6 particles
	// in each
	fpoint ls=1.8*pow(bx*by*bz,-1.0/3.0);
	fpoint nxf=bx*ls+1.5;
	fpoint nyf=by*ls+1.5;
	fpoint nzf=bz*ls+1.5;

	// Check the grid is not too huge, using floating point numbers to avoid
	// integer wrap-arounds
	if (nxf*nyf*nzf>max_regions) {
		cerr << "voro++: Number of computational blocks exceeds the maximum allowed of " << max_regions << ".\n";
		cerr << "Either increase the particle length scale, or recompile with an increased\nmaximum." << endl;
		return VOROPP_MEMORY_ERROR;
	}

	// Now that we are confident that the number of regions is reasonable,
	// create integer versions of them
	int nx=int(nxf);
	int ny=int(nyf);
	int nz=int(nzf);
	printf("Total particles = %d\n\nInternal grid size = (%d %d %d)\n\n",n,nx,ny,nz);

	if(radial) {

		// Create a container with the geometry given above
		container_periodic_poly con(bx,bxy,by,bxz,byz,bz,nx,ny,nz,memory);

		// Read in the particles from the file
		for(i=0;i<n;i++) {
			is >> buffer;
			is >> x >> y >> z;
			con.put(i,x,y,z,radial_lookup(buffer));
		}

		// Carry out the volume check
		printf("Volume check:\n  Total domain volume  = %f\n",bx*by*bz);
		printf("  Total Voronoi volume = %f\n",con.sum_cell_volumes());

		// Copy the output filename
		for(i=0;i<bp-2;i++) buffer[i]=farg[i];
		output(con,buffer,bp);
	} else {

		// Create a container with the geometry given above
		container_periodic con(bx,bxy,by,bxz,byz,bz,nx,ny,nz,memory);

		// Read in the particles from the file
		for(i=0;i<n;i++) {
			is >> buffer;
			is >> x >> y >> z;
			con.put(i,x,y,z);
		}

		// Carry out the volume check
		printf("Volume check:\n  Total domain volume  = %f\n",bx*by*bz);
		printf("  Total Voronoi volume = %f\n",con.sum_cell_volumes());;

		// Copy the output filename
		for(i=0;i<bp-2;i++) buffer[i]=farg[i];
		output(con,buffer,bp);
	}
}

inline void extension(const char *ext,char *bu) {
	char *ep((char*) ext);
	while(*ep!=0) *(bu++)=*(ep++);*bu=*ep;	
}

template<class c_option>
void output(c_option &con,char *buffer,int bp) {
	char *bu(buffer+bp-2);
	voronoi_network vn(con);

	// Compute non-rectangular cell network, and print gnuplot and network
	// files
	con.compute_network(vn);
	extension("nd2",bu);vn.draw_network(buffer);
	extension("nt2",bu);vn.print_network(buffer);

	// Compute rectangular cell network, and print gnuplot and network
	// files
	con.compute_network_rectangular(vn);
	extension("ntd",bu);vn.draw_network(buffer);
	extension("net",bu);vn.print_network(buffer);

	// Output the particles and any constructed periodic images
	extension("par",bu);con.draw_particles(buffer);

	// Output the Voronoi cells in gnuplot format
	extension("out",bu);con.draw_cells_gnuplot(buffer);
	
	// Output the unit cell in gnuplot format
	extension("dom",bu);con.draw_domain(buffer);
}
