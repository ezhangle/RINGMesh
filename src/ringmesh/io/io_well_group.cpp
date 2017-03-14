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

#include <ringmesh/io/io.h>

#include <geogram/basic/file_system.h>
#include <geogram/basic/line_stream.h>

#include <ringmesh/geomodel/geomodel.h>

#include <ringmesh/mesh/well.h>

#include <ringmesh/mesh/mesh_builder.h>
#include <ringmesh/mesh/geogram_mesh.h>

/*!
 * @file Implements the input - output of WellGroup
 * @author Arnaud Botella
 */

namespace {
    using namespace RINGMesh ;

    static std::string TAB = "\t" ;
    static std::string SPACE = " " ;

    void merge_colocated_vertices( double epsilon, Mesh1D& mesh )
    {
        std::vector< index_t > old2new ;
        index_t nb_colocated = mesh.vertices_nn_search().get_colocated_index_mapping(
            epsilon, old2new ) ;
        DEBUG( nb_colocated ) ;
        if( nb_colocated > 0 ) {
            Mesh1DBuilder_var builder = Mesh1DBuilder::create_builder( mesh ) ;
            for( index_t e = 0; e < mesh.nb_edges(); e++ ) {
                for( index_t i = 0; i < 2; i++ ) {
                    index_t v = mesh.edge_vertex( e, i ) ;
                    builder->set_edge_vertex( e, i, old2new[v] ) ;
                }
            }
            std::vector< bool > delete_vertices( mesh.nb_vertices(), false ) ;
            for( index_t v = 0; v < mesh.nb_vertices(); v++ ) {
                if( old2new[v] != v ) {
                    delete_vertices[v] = true ;
                }
            }
            builder->delete_vertices( delete_vertices ) ;
        }
    }

    class WLIOHandler: public WellGroupIOHandler {
    public:
        virtual void load( const std::string& filename, WellGroup& wells )
        {
            GEO::LineInput in( filename ) ;
            if( !in.OK() ) {
                throw RINGMeshException( "I/O", "Could not open file" ) ;
            }

            Mesh1D* mesh = Mesh1D::create_mesh( GeogramMesh1D::type_name_static() ) ;
            Mesh1DBuilder* builder = Mesh1DBuilder::create_builder( *mesh ) ;
            std::string name ;
            double z_sign = 1.0 ;
            vec3 vertex_ref ;

            while( !in.eof() ) {
                in.get_line() ;
                in.get_fields() ;
                if( in.nb_fields() == 0 ) continue ;
                if( in.field_matches( 0, "name:" ) ) {
                    name = in.field( 1 ) ;
                } else if( in.field_matches( 0, "ZPOSITIVE" ) ) {
                    if( in.field_matches( 1, "Depth" ) ) {
                        z_sign = -1.0 ;
                    }
                } else if( in.field_matches( 0, "WREF" ) ) {
                    vertex_ref[0] = in.field_as_double( 1 ) ;
                    vertex_ref[1] = in.field_as_double( 2 ) ;
                    vertex_ref[2] = z_sign * in.field_as_double( 3 ) ;
                    builder->create_vertex( vertex_ref ) ;
                } else if( in.field_matches( 0, "PATH" ) ) {
                    if( in.field_as_double( 1 ) == 0. ) continue ;
                    vec3 vertex ;
                    vertex[2] = z_sign * in.field_as_double( 2 ) ;
                    vertex[0] = in.field_as_double( 3 ) + vertex_ref[0] ;
                    vertex[1] = in.field_as_double( 4 ) + vertex_ref[1] ;
                    index_t id = builder->create_vertex( vertex ) ;
                    builder->create_edge( id - 1, id ) ;
                } else if( in.field_matches( 0, "END" ) ) {
                    wells.add_well( *mesh, name ) ;
                    delete mesh ;
                    delete builder ;
                    mesh = Mesh1D::create_mesh( GeogramMesh1D::type_name_static() ) ;
                    builder = Mesh1DBuilder::create_builder( *mesh ) ;
                }
            }

            delete mesh ;
            delete builder ;
        }
        virtual void save( const WellGroup& wells, const std::string& filename )
        {
            ringmesh_unused( wells ) ;
            ringmesh_unused( filename ) ;
            throw RINGMeshException( "I/O",
                "Saving of a WellGroup from Gocad not implemented yet" ) ;
        }
    } ;

    class SmeshIOHandler: public WellGroupIOHandler {
    public:
        virtual void load( const std::string& filename, WellGroup& wells )
        {
            GEO::LineInput in( filename ) ;
            if( !in.OK() ) {
                throw RINGMeshException( "I/O", "Could not open file" ) ;
            }

            Mesh1D* mesh = Mesh1D::create_mesh( GeogramMesh1D::type_name_static() ) ;
            Mesh1DBuilder* builder = Mesh1DBuilder::create_builder( *mesh ) ;
            std::string name = GEO::FileSystem::base_name( filename ) ;

            bool is_first_part = true ;

            while( !in.eof() ) {
                in.get_line() ;
                in.get_fields() ;
                if( in.nb_fields() == 0 ) continue ;
                if( GEO::String::string_starts_with( in.field( 0 ), "#" ) ) {
                    continue ;
                }
                if( is_first_part ) {
                    index_t nb_vertices = in.field_as_uint( 0 ) ;
                    builder->create_vertices( nb_vertices ) ;
                    Box3d box ;

                    for( index_t v = 0; v < nb_vertices; v++ ) {
                        do {
                            in.get_line() ;
                            in.get_fields() ;
                        } while( in.nb_fields() == 0 ) ;
                        vec3 point ;
                        point[0] = in.field_as_double( 1 ) ;
                        point[1] = in.field_as_double( 2 ) ;
                        point[2] = in.field_as_double( 3 ) ;
                        builder->set_vertex( v, point ) ;
                        box.add_point( point ) ;
                    }
                    is_first_part = false ;
                } else {
                    index_t nb_edges = in.field_as_uint( 0 ) ;
                    builder->create_edges( nb_edges ) ;
                    for( index_t e = 0; e < nb_edges; e++ ) {
                        do {
                            in.get_line() ;
                            in.get_fields() ;
                        } while( in.nb_fields() == 0 ) ;
                        builder->set_edge_vertex( e, 0, in.field_as_uint( 1 ) ) ;
                        builder->set_edge_vertex( e, 1, in.field_as_uint( 2 ) ) ;
                    }
                    merge_colocated_vertices( wells.geomodel()->epsilon(), *mesh ) ;
                    mesh->save_mesh( "test.geogram" ) ;
                    index_t count = 0 ;
                    for( index_t e = 0; e < mesh->nb_edges(); e++ ) {
                        if( mesh->edge_length( e ) < wells.geomodel()->epsilon() ) count++ ;
                    }
                    DEBUG(count);
                    wells.add_well( *mesh, name ) ;
                    break ;
                }
            }
            delete mesh ;
            delete builder ;
        }
        virtual void save( const WellGroup& wells, const std::string& filename )
        {
            ringmesh_unused( wells ) ;
            ringmesh_unused( filename ) ;
            throw RINGMeshException( "I/O",
                "Saving of a WellGroup from Smesh not implemented yet" ) ;
        }
    } ;

}

namespace RINGMesh {
    /*!
     * Loads a WellGroup from a file
     * @param[in] filename the file to load
     * @param][out] wells the wells to fill
     */
    void well_load( const std::string& filename, WellGroup& wells )
    {
        Logger::out( "I/O" ) << "Loading file " << filename << "..." << std::endl ;

        WellGroupIOHandler_var handler = WellGroupIOHandler::get_handler(
            filename ) ;
        handler->load( filename, wells ) ;
    }

    WellGroupIOHandler* WellGroupIOHandler::create( const std::string& format )
    {
        WellGroupIOHandler* handler = WellGroupIOHandlerFactory::create_object(
            format ) ;
        if( !handler ) {
            std::vector< std::string > names ;
            WellGroupIOHandlerFactory::list_creators( names ) ;
            Logger::err( "I/O" ) << "Currently supported file formats are: " ;
            for( index_t i = 0; i < names.size(); i++ ) {
                Logger::err( "I/O" ) << " " << names[i] ;
            }
            Logger::err( "I/O" ) << std::endl ;

            throw RINGMeshException( "I/O", "Unsupported file format: " + format ) ;
        }
        return handler ;
    }

    WellGroupIOHandler* WellGroupIOHandler::get_handler(
        const std::string& filename )
    {
        std::string ext = GEO::FileSystem::extension( filename ) ;
        return create( ext ) ;
    }

    /*
     * Initializes the possible handler for IO files
     */
    void WellGroupIOHandler::initialize()
    {
        ringmesh_register_WellGroupIOHandler_creator( WLIOHandler, "wl" ) ;
        ringmesh_register_WellGroupIOHandler_creator( SmeshIOHandler, "smesh" ) ;
    }
}