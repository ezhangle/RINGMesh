/*
 * Copyright (c) 2012-2017, Association Scientifique pour la Geologie et ses Applications (ASGA)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of ASGA nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ASGA BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *     http://www.ring-team.org
 *
 *     RING Project
 *     Ecole Nationale Superieure de Geologie - GeoRessources
 *     2 Rue du Doyen Marcel Roubault - TSA 70605
 *     54518 VANDOEUVRE-LES-NANCY
 *     FRANCE
 */

#include <ringmesh/tetrahedralize/tetgen_mesher.h>

#include <cstring>

#include <geogram/mesh/mesh.h>

#ifdef RINGMESH_WITH_TETGEN
/*!
 * @file Implementation of the interface of GEO::Mesh with Tetgen
 * @author Jeanne Pellerin
 */

namespace RINGMesh {

    using GEO::Mesh ;

    bool is_mesh_tetrahedralizable( const Mesh& M )
    {
        if( M.facets.nb() == 0 ) {
            Logger::err( "RING" ) << "Mesh to tetrahedralize has no facets "
                << std::endl ;
            return false ;
        }
        if( !M.facets.are_simplices() ) {
            Logger::err( "RING" ) << "Mesh to tetrahedralize is not triangulated"
                << std::endl ;
            return false ;
        }
        if( M.cells.nb() != 0 ) {
            Logger::warn( "RING" ) << "Mesh to tetrahedralize already have cells"
                << std::endl ;
        }
        return true ;
    }

    TetgenMesher::~TetgenMesher()
    {
        // Take over facet deletion of tetgen that does not set to 
        // nil pointers to polygonlist or holelist in facet
        delete[] tetgen_in_.facetlist ;
        tetgen_in_.facetlist = nil ;
        tetgen_in_.numberoffacets = 0 ;

        delete[] polygon_corners_ ;
        polygon_corners_ = nil ;

        delete[] polygons_ ;
        polygons_ = nil ;
    }

    void TetgenMesher::tetrahedralize(
        const Mesh& input_mesh,
        Mesh& output_mesh )
    {
        initialize() ;
        copy_mesh_to_tetgen_input( input_mesh ) ;
        tetrahedralize() ;
        assign_result_tetmesh_to_mesh( output_mesh ) ;
    }


    void TetgenMesher::initialize()
    {
        initialize_tetgen_args() ;
        tetgen_in_.initialize() ;
        tetgen_out_.initialize() ;
    }

    void TetgenMesher::tetrahedralize()
    {
        try {
            GEO_3rdParty::tetrahedralize( &tetgen_args_, &tetgen_in_,
                &tetgen_out_ ) ;
        } catch( int code ) {
            Logger::err( "Tetgen" ) << "Encountered a problem: " ;
            switch( code ) {
                case 1:
                    Logger::err( "Tetgen" ) << "Out of memory" ;
                    break ;
                case 2:
                    Logger::err( "Tetgen" )
                        << "Please report this bug to Hang.Si@wias-berlin.de. Include\n" ;
                    Logger::err( "Tetgen" )
                        << "  the message above, your input data set, and the exact\n" ;
                    Logger::err( "Tetgen" )
                        << "  command line you used to run this program, thank you" ;
                    break ;
                case 3:
                    Logger::err( "Tetgen" )
                        << "A self-intersection was detected. Program stopped\n" ;
                    Logger::err( "Tetgen" )
                        << "Hint: use -d option to detect all self-intersections" ;
                    break ;
                case 4:
                    Logger::err( "Tetgen" )
                        << "A very small input feature size was detected. Program stopped.\n" ;
                    Logger::err( "Tetgen" )
                        << "Hint: use -T option to set a smaller tolerance." ;
                    break ;
                case 5:
                    Logger::err( "Tetgen" )
                        << "Two very close input facets were detected. Program stopped.\n" ;
                    Logger::err( "Tetgen" )
                        << "Hint: use -Y option to avoid adding Steiner points in boundary." ;
                    break ;
                case 10:
                    Logger::err( "Tetgen" )
                        << "An input error was detected. Program stopped." ;
                    break ;
            }
            Logger::err( "Tetgen" ) << std::endl ;
        }
    }

    void TetgenMesher::copy_mesh_to_tetgen_input( const Mesh& M )
    {
        if( M.vertices.nb() != 0 ) {
            copy_vertices_to_tetgen_input( M ) ;
        }
        if( M.edges.nb() != 0 ) {
            copy_edges_to_tetgen_input( M ) ;
        }
        if( M.facets.nb() != 0 ) {
            copy_facets_to_tetgen_input( M ) ;
        }
    }

    void TetgenMesher::copy_vertices_to_tetgen_input( const Mesh& M )
    {
        tetgen_in_.numberofpoints = static_cast< int >( M.vertices.nb() ) ;
        tetgen_in_.pointlist = new double[3 * tetgen_in_.numberofpoints] ;
        GEO::Memory::copy( tetgen_in_.pointlist, M.vertices.point_ptr( 0 ),
            M.vertices.nb() * 3 * sizeof(double) ) ;
    }

    void TetgenMesher::copy_edges_to_tetgen_input( const Mesh& M )
    {
        tetgen_in_.numberofedges = static_cast< int >( M.edges.nb() ) ;
        tetgen_in_.edgelist = new int[2 * tetgen_in_.numberofedges] ;
        GEO::Memory::copy( tetgen_in_.edgelist, M.edges.vertex_index_ptr( 0 ),
            M.edges.nb() * 2 * sizeof(int) ) ;
    }

    void TetgenMesher::copy_facets_to_tetgen_input( const Mesh& M )
    {
        polygons_ = new GEO_3rdParty::tetgenio::polygon[M.facets.nb()] ;

        tetgen_in_.numberoffacets = static_cast< int >( M.facets.nb() ) ;
        tetgen_in_.facetlist =
            new GEO_3rdParty::tetgenio::facet[tetgen_in_.numberoffacets] ;

        polygon_corners_ = new int[M.facet_corners.nb()] ;
        GEO::Memory::copy( polygon_corners_, M.facet_corners.vertex_index_ptr( 0 ),
            M.facet_corners.nb() * sizeof(int) ) ;

        for( index_t f = 0; f < M.facets.nb(); ++f ) {
            GEO_3rdParty::tetgenio::facet& F = tetgen_in_.facetlist[f] ;
            GEO_3rdParty::tetgenio::init( &F ) ;
            F.numberofpolygons = 1 ;
            F.polygonlist = &polygons_[f] ;

            GEO_3rdParty::tetgenio::polygon& P = F.polygonlist[0] ;
            GEO_3rdParty::tetgenio::init( &P ) ;
            P.numberofvertices = static_cast< int >( M.facets.nb_corners( f ) ) ;
            P.vertexlist = &polygon_corners_[M.facets.corners_begin( f )] ;
        }
    }

    void TetgenMesher::set_regions(
        const std::vector< vec3 >& one_point_in_each_region )
    {
        index_t nb_regions = one_point_in_each_region.size() ;
        tetgen_in_.numberofregions = static_cast< int >( nb_regions ) ;
        tetgen_in_.regionlist = new double[5 * nb_regions] ;

        for( index_t i = 0; i != nb_regions; ++i ) {
            tetgen_in_.regionlist[5 * i] = one_point_in_each_region[i].x ;
            tetgen_in_.regionlist[5 * i + 1] = one_point_in_each_region[i].y ;
            tetgen_in_.regionlist[5 * i + 2] = one_point_in_each_region[i].z ;
            tetgen_in_.regionlist[5 * i + 3] = i ;
            tetgen_in_.regionlist[5 * i + 4] = DBL_MAX ; // Used only with the a switch
        }
    }

    void TetgenMesher::initialize_tetgen_args()
    {
        char* copy = new char[tetgen_command_line_.length() + 1] ;
        std::strcpy( copy, tetgen_command_line_.c_str() ) ;
        tetgen_args_.parse_commandline( copy ) ;
        delete[] copy ;
    }

    void TetgenMesher::assign_result_tetmesh_to_mesh( Mesh& M ) const
    {
        GEO::vector< double > points ;
        get_result_tetmesh_points( points ) ;

        GEO::vector< index_t > tets ;
        get_result_tetmesh_tets( tets ) ;

        M.cells.assign_tet_mesh( 3, points, tets, true ) ;
        M.vertices.remove_isolated() ;
        M.cells.connect() ;
    }

    void TetgenMesher::get_result_tetmesh_points(
        GEO::vector< double >& points ) const
    {
        index_t nb_points = static_cast< index_t >( tetgen_out_.numberofpoints ) ;
        points.resize( 3 * nb_points ) ;
        double* points_ptr = tetgen_out_.pointlist ;
        RINGMESH_PARALLEL_LOOP
        for( index_t i = 0; i < 3 * nb_points; ++i ) {
            points[i] = points_ptr[i] ;
        }
    }

    void TetgenMesher::get_result_tetmesh_tets( GEO::vector< index_t >& tets ) const
    {
        std::vector< index_t > tets_to_keep ;
        determine_tets_to_keep( tets_to_keep ) ;

        index_t nb_tets = static_cast< index_t >( tets_to_keep.size() ) ;
        tets.resize( 4 * nb_tets ) ;
        int* tets_ptr = tetgen_out_.tetrahedronlist ;
        RINGMESH_PARALLEL_LOOP
        for( index_t i = 0; i < nb_tets; ++i ) {
            index_t tetra = tets_to_keep[i] ;
            for( index_t v = 0; v < 4; v++ ) {
                tets[4 * i + v] = static_cast< index_t >( tets_ptr[4 * tetra + v] ) ;
            }
        }
    }

    void TetgenMesher::determine_tet_regions_to_keep(
        std::set< double >& regions_to_keep ) const
    {
        // Determine which regions are incident to
        // the 'exterior' (neighbor = -1).
        // The region Id of tet t is determined by:
        //  tetgen_out_.tetrahedronattributelist[t]

        index_t nb_tets = static_cast< index_t >( tetgen_out_.numberoftetrahedra ) ;
        for( index_t t = 0; t < nb_tets; ++t ) {
            for( index_t f = 0; f < 4; ++f ) {
                signed_index_t n = tetgen_out_.neighborlist[t * 4 + f] ;
                if( n == -1 ) {
                    regions_to_keep.insert( tetgen_out_.tetrahedronattributelist[t] ) ;
                    break ;
                }
            }
        }
    }

    void TetgenMesher::determine_tets_to_keep(
        std::vector< index_t >& tets_to_keep ) const
    {
        std::set< double > regions_to_keep ;
        determine_tet_regions_to_keep( regions_to_keep ) ;

        index_t nb_tets = static_cast< index_t >( tetgen_out_.numberoftetrahedra ) ;
        tets_to_keep.reserve( nb_tets ) ;
        for( index_t t = 0; t < nb_tets; ++t ) {
            if( regions_to_keep.find( tetgen_out_.tetrahedronattributelist[t] )
                != regions_to_keep.end() ) {
                tets_to_keep.push_back( t ) ;
            }
        }
    }

    void tetrahedralize_mesh_tetgen( Mesh& M, bool refine, double quality )
    {
        if( !is_mesh_tetrahedralizable( M ) ) {
            throw RINGMeshException( "TetGen", "Mesh cannot be tetrahedralized" ) ;
        }
        TetgenMesher mesher ;
        if( refine ) {
            mesher.add_points_to_match_quality( quality ) ;
        }
        mesher.tetrahedralize( M, M ) ;
    }

    void TetgenMesher::add_points_to_match_quality( double quality ) {
        tetgen_command_line_ += "q" + GEO::String::to_string( quality ) ;
    }
}

#endif
