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

#include <ringmesh/geomodel/geomodel_builder_repair.h>

#include <ringmesh/geomodel/geomodel_builder.h>

/*!
 * @file Implementation of repair function of the surfaces of a GeoModel
 * @author Jeanne Pellerin
 * @author Pierre Anquez
 */

namespace RINGMesh {

    GeoModelBuilderRepair::GeoModelBuilderRepair(
        GeoModelBuilder& builder,
        GeoModel& geomodel )
        : builder_( builder ), geomodel_( geomodel ), geomodel_access_( geomodel )
    {
    }

    void GeoModelBuilderRepair::repair( RepairMode repair_mode )
    {
        switch( repair_mode ) {
            case ALL:
                geomodel_mesh_repair() ;
                break ;
            case BASIC:
                builder_.end_geomodel() ;
                break ;
            case COLOCATED_VERTICES:
                remove_colocated_entity_vertices_and_update_gm() ;
                break ;
            case DEGENERATE_FACETS_EDGES:
                remove_degenerate_facets_and_edges_and_update_gm() ;
                break ;
            case LINE_BOUNDARY_ORDER:
                repair_line_boundary_vertex_order() ;
                break ;
            default:
                ringmesh_assert_not_reached ;
        }
    }

    void GeoModelBuilderRepair::geomodel_mesh_repair()
    {
        // Remove colocated vertices in each entity
        remove_colocated_entity_vertices_and_update_gm() ;

        // Basic mesh repair for surfaces and lines
        remove_degenerate_facets_and_edges_and_update_gm() ;

        // Proper reordering of line boundaries
        repair_line_boundary_vertex_order() ;

        // This is basic requirement ! no_colocated geomodel vertices !
        // So remove them if there are any
        geomodel_.mesh.remove_colocated_vertices() ;

        builder_.end_geomodel() ;
    }

    void GeoModelBuilderRepair::remove_colocated_entity_vertices_and_update_gm()
    {
        std::set< gme_t > empty_entities ;
        remove_colocated_entity_vertices( empty_entities ) ;
        if( !empty_entities.empty() ) {
            builder_.topology.get_dependent_entities( empty_entities ) ;
            builder_.removal.remove_mesh_entities( empty_entities ) ;
        }
    }

    void GeoModelBuilderRepair::remove_degenerate_facets_and_edges_and_update_gm()
    {
        std::set< gme_t > empty_entities ;
        remove_degenerate_facets_and_edges( empty_entities ) ;
        /// TODO when it will works,
        /// use GeoModelBuilderRemoval::remove_entities_and_dependencies
        if( !empty_entities.empty() ) {
            builder_.removal.remove_mesh_entities( empty_entities ) ;
        }

        // This is basic requirement ! no_colocated geomodel vertices !
        // So remove them if there are any
        geomodel_.mesh.remove_colocated_vertices() ;

        builder_.end_geomodel() ;
    }

    void GeoModelBuilderRepair::repair_line_boundary_vertex_order()
    {
        for( index_t line_itr = 0; line_itr < geomodel_.nb_lines(); ++line_itr ) {
            const Line& cur_line = geomodel_.line( line_itr ) ;
            if( !cur_line.is_first_corner_first_vertex() ) {
                const index_t first_boundary_index = cur_line.boundary( 0 ).index() ;
                builder_.topology.set_mesh_entity_boundary( cur_line.gme_id(), 0,
                    cur_line.boundary_gme( 1 ).index ) ;
                builder_.topology.set_mesh_entity_boundary( cur_line.gme_id(), 1,
                    first_boundary_index ) ;
            }
        }
    }

    bool GeoModelBuilderRepair::facet_is_degenerate(
        const Surface& S,
        index_t f,
        std::vector< index_t >& colocated_vertices )
    {
        index_t nb_vertices = S.nb_mesh_element_vertices( f ) ;
        if( nb_vertices != 3 ) {
            index_t* vertices = (index_t*) alloca( nb_vertices * sizeof(index_t) ) ;
            for( index_t lv = 0; lv < nb_vertices; ++lv ) {
                vertices[lv] = colocated_vertices[S.mesh_element_vertex_index( f,
                    lv )] ;
            }
            std::sort( vertices, vertices + nb_vertices ) ;
            return std::unique( vertices, vertices + nb_vertices )
                != vertices + nb_vertices ;
        }
        index_t v1 = colocated_vertices[S.mesh_element_vertex_index( f, 0 )] ;
        index_t v2 = colocated_vertices[S.mesh_element_vertex_index( f, 1 )] ;
        index_t v3 = colocated_vertices[S.mesh_element_vertex_index( f, 2 )] ;
        return v1 == v2 || v2 == v3 || v3 == v1 ;
    }

    void GeoModelBuilderRepair::surface_detect_degenerate_facets(
        const Surface& S,
        std::vector< index_t >& f_is_degenerate,
        std::vector< index_t >& colocated_vertices )
    {
        f_is_degenerate.resize( S.nb_mesh_elements() ) ;
        for( index_t f = 0; f < S.nb_mesh_elements(); ++f ) {
            f_is_degenerate[f] = facet_is_degenerate( S, f, colocated_vertices ) ;
        }
    }

    index_t GeoModelBuilderRepair::detect_degenerate_facets( const Surface& S )
    {
        std::vector< index_t > colocated ;
        const NNSearch& nn_search = S.vertex_nn_search() ;
        nn_search.get_colocated_index_mapping( geomodel_.epsilon(), colocated ) ;

        std::vector< index_t > degenerate ;
        surface_detect_degenerate_facets( S, degenerate, colocated ) ;
        return static_cast< index_t >( std::count( degenerate.begin(),
            degenerate.end(), 1 ) ) ;
    }

    void GeoModelBuilderRepair::line_detect_degenerate_edges(
        const Line& L,
        std::vector< bool >& e_is_degenerate,
        std::vector< index_t >& colocated_vertices )
    {
        e_is_degenerate.resize( L.nb_mesh_elements() ) ;
        for( index_t e = 0; e < L.nb_mesh_elements(); ++e ) {
            e_is_degenerate[e] = edge_is_degenerate( L, e, colocated_vertices ) ;
        }
    }

    index_t GeoModelBuilderRepair::repair_line_mesh( const Line& line )
    {
        std::vector< index_t > colocated ;
        const NNSearch& nn_search = line.vertex_nn_search() ;
        nn_search.get_colocated_index_mapping( geomodel_.epsilon(), colocated ) ;

        std::vector< bool > degenerate ;
        line_detect_degenerate_edges( line, degenerate, colocated ) ;
        index_t nb = static_cast< index_t >( std::count( degenerate.begin(),
            degenerate.end(), 1 ) ) ;
        /// We have a problem if some vertices are left isolated
        /// If we remove them here we can kill all indices correspondances
        builder_.geometry.delete_line_edges( line.index(), degenerate, false ) ;
        return nb ;
    }

    void GeoModelBuilderRepair::remove_degenerate_facets_and_edges(
        std::set< gme_t >& to_remove )
    {
        to_remove.clear() ;
        for( index_t i = 0; i < geomodel_.nb_lines(); ++i ) {
            const Line& line = geomodel_.line( i ) ;
            index_t nb = repair_line_mesh( line ) ;
            if( nb > 0 ) {
                Logger::out( "GeoModel" ) << nb
                    << " degenerated edges removed in LINE " << i << std::endl ;
                // If the Line is set it to remove
                if( geomodel_.line( i ).nb_mesh_elements() == 0 ) {
                    to_remove.insert( geomodel_.line( i ).gme_id() ) ;
                }
            }
        }
        // The builder might be needed

        double epsilon_sq = geomodel_.epsilon() * geomodel_.epsilon() ;
        for( index_t i = 0; i < geomodel_.nb_surfaces(); ++i ) {
            const Surface& surface = geomodel_.surface( i ) ;
            index_t nb = detect_degenerate_facets( surface ) ;
            /// @todo Check if that cannot be simplified
            if( nb > 0 ) {
                // If there are some degenerated facets
                // Using repair function of geogram
                // Warning - This triangulates the mesh
                if( surface.nb_vertices() > 0 ) {
                    // Colocated vertices must be processed before
                    // MESH_REPAIR_DUP_F 2 ;
                    GEO::MeshRepairMode mode =
                        static_cast< GEO::MeshRepairMode >( 2 ) ;
                    Mesh2DBuilder_var builder =
                        builder_.geometry.create_surface_builder( i ) ;
                    builder->mesh_repair( mode, 0.0 ) ;

                    // This might create some small components - remove them
                    builder->remove_small_connected_components( epsilon_sq, 3 ) ;

                    // Alright, this is a bit of an overkill [JP]
                    if( surface.nb_vertices() > 0 ) {
                        builder->mesh_repair( mode, 0.0 ) ;
                    }
                }
                if( surface.nb_vertices() == 0 || surface.nb_mesh_elements() == 0 ) {
                    to_remove.insert( geomodel_.surface( i ).gme_id() ) ;
                }
            }
        }
    }

    void GeoModelBuilderRepair::vertices_on_inside_boundary(
        const gme_t& E_id,
        std::set< index_t >& vertices )
    {
        vertices.clear() ;
        if( E_id.type == Corner::type_name_static() ) {
            return ;
        }
        const GeoModelMeshEntity& E = geomodel_.mesh_entity( E_id ) ;
        if( E_id.type == Line::type_name_static() ) {
            if( E.boundary( 0 ).is_inside_border( E ) ) {
                vertices.insert( E.nb_vertices() - 1 ) ;
            }
            return ;
        }
        std::vector< const GeoModelMeshEntity* > inside_border ;
        for( index_t i = 0; i < E.nb_boundaries(); ++i ) {
            if( E.boundary( i ).is_inside_border( E ) ) {
                inside_border.push_back(
                    dynamic_cast< const GeoModelMeshEntity* >( &E.boundary( i ) ) ) ;
            }
        }
        if( !inside_border.empty() ) {
            // We want to get the indices of the vertices in E
            // that are colocated with those of the inside boundary
            // We assume that the geomodel vertices are not computed
            const NNSearch& nn_search = E.vertex_nn_search() ;

            for( index_t i = 0; i < inside_border.size(); ++i ) {
                for( index_t v = 0; v < inside_border[i]->nb_vertices(); ++v ) {
                    std::vector< index_t > colocated_indices ;
                    nn_search.get_neighbors( inside_border[i]->vertex( v ),
                        colocated_indices, geomodel_.epsilon() ) ;
                    if( colocated_indices.size() > 1 ) {
                        std::sort( colocated_indices.begin(),
                            colocated_indices.end() ) ;
                        // Add colocated vertices except one to the duplicated vertices set
                        vertices.insert( colocated_indices.begin() + 1,
                            colocated_indices.end() ) ;
                    }
                }
            }
        }
    }

    void GeoModelBuilderRepair::remove_colocated_entity_vertices(
        std::set< gme_t >& to_remove )
    {
        to_remove.clear() ;
        // For all Lines and Surfaces
        const std::string types[2] = {
            Line::type_name_static(), Surface::type_name_static() } ;
        for( index_t t = 0; t < 2; ++t ) {
            const std::string& T = types[t] ;

            for( index_t e = 0; e < geomodel_.nb_mesh_entities( T ); ++e ) {
                gme_t entity_id( T, e ) ;
                const GeoModelMeshEntity& E = geomodel_.mesh_entity( entity_id ) ;

                const NNSearch& kdtree = E.vertex_nn_search() ;
                std::vector< index_t > colocated ;
                kdtree.get_colocated_index_mapping( geomodel_.epsilon(),
                    colocated ) ;

                // Get the vertices to delete
                std::set< index_t > inside_border ;
                vertices_on_inside_boundary( entity_id, inside_border ) ;

                std::vector< bool > to_delete( colocated.size(), false ) ;
                index_t nb_todelete = 0 ;
                for( index_t v = 0; v < colocated.size(); ++v ) {
                    if( colocated[v] == v
                        || inside_border.find( v ) != inside_border.end() ) {
                        // This point is kept
                        // No colocated or on an inside boundary
                    } else {
                        // The point is to remove
                        to_delete[v] = true ;
                        nb_todelete++ ;
                    }
                }

                if( nb_todelete == 0 ) {
                    // Nothing to do there
                    continue ;
                } else if( nb_todelete == E.nb_vertices() ) {
                    // The complete entity should be removed
                    to_remove.insert( E.gme_id() ) ;
                    continue ;
                } else {
                    if( t == 1 ) {
                        Mesh2DBuilder_var builder =
                            builder_.geometry.create_surface_builder( e ) ;
                        for( index_t f_itr = 0; f_itr < E.nb_mesh_elements();
                            f_itr++ ) {
                            for( index_t fv_itr = 0;
                                fv_itr < E.nb_mesh_element_vertices( f_itr );
                                fv_itr++ ) {
                                builder->set_facet_vertex( f_itr, fv_itr,
                                    colocated[E.mesh_element_vertex_index( f_itr,
                                        fv_itr )] ) ;
                            }
                        }
                        builder->delete_vertices( to_delete ) ;
                        Logger::out( "Repair" ) << nb_todelete
                            << " colocated vertices deleted in " << entity_id
                            << std::endl ;

                    } else if( t == 0 ) {
                        Mesh1DBuilder_var builder =
                            builder_.geometry.create_line_builder( e ) ;
                        for( index_t e_itr = 0; e_itr < E.nb_mesh_elements();
                            e_itr++ ) {
                            builder->set_edge_vertex( e_itr, 0,
                                colocated[E.mesh_element_vertex_index( e_itr, 0 )] ) ;
                            builder->set_edge_vertex( e_itr, 1,
                                colocated[E.mesh_element_vertex_index( e_itr, 1 )] ) ;
                        }
                        builder->delete_vertices( to_delete ) ;
                        Logger::out( "Repair" ) << nb_todelete
                            << " colocated vertices deleted in " << entity_id
                            << std::endl ;
                    } else {
                        ringmesh_assert_not_reached ;
                    }
                }
            }
        }
    }

    bool GeoModelBuilderRepair::edge_is_degenerate(
        const Line& L,
        index_t e,
        const std::vector< index_t >& colocated_vertices )
    {
        index_t v1 = colocated_vertices[L.mesh_element_vertex_index( e, 0 )] ;
        index_t v2 = colocated_vertices[L.mesh_element_vertex_index( e, 1 )] ;
        return v1 == v2 ;
    }

}