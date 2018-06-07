/*
 * Copyright (c) 2018, Association Scientifique pour la Geologie et ses
 * Applications (ASGA). All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ASGA BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *     http://www.ring-team.org
 *
 *     RING Project
 *     Ecole Nationale Superieure de Geologie - GeoRessources
 *     2 Rue du Doyen Marcel Roubault - TSA 70605
 *     54518 VANDOEUVRE-LES-NANCY
 *     FRANCE
 */

#include <geogram/basic/attributes.h>

#include <ringmesh/basic/geometry.h>
#include <ringmesh/geomodel/core/geomodel.h>
#include <ringmesh/geomodel/core/geomodel_api.h>

#include <ringmesh/geomodel/core/geomodel_mesh_entity.h>
#include <ringmesh/io/geomodel_builder_resqml.h>

#include <ringmesh/mesh/mesh_index.h>

#include <ringmesh/mesh/surface_mesh.h>
#include <ringmesh/mesh/volume_mesh.h>

#include <ringmesh/mesh/mesh_builder.h>

#include <fesapi/common/EpcDocument.h>
#include <fesapi/common/HdfProxy.h>
#include <fesapi/resqml2_0_1/LocalDepth3dCrs.h>
#include <fesapi/resqml2_0_1/LocalTime3dCrs.h>
#include <fesapi/resqml2_0_1/TriangulatedSetRepresentation.h>
#include <fesapi/resqml2_0_1/UnstructuredGridRepresentation.h>
/*!
 * @brief Implementation of the class to build GeoModel from input
 * RESQML2 .epc file
 * @author Wan-Chiu Li
 */

namespace RINGMesh
{
    using namespace RESQML2_0_1_NS;

    namespace
    {
        void showAllMetadata(
            COMMON_NS::AbstractObject* obj, const std::string& prefix = "" )
        {
            std::cout << prefix << "Title is : " << obj->getTitle()
                      << std::endl;
            std::cout << prefix << "Guid is : " << obj->getUuid() << std::endl;
            if( !obj->isPartial() )
            {
                for( unsigned int i = 0; i < obj->getAliasCount(); ++i )
                {
                    std::cout
                        << prefix
                        << "Alias is : " << obj->getAliasAuthorityAtIndex( i )
                        << ":" << obj->getAliasTitleAtIndex( i ) << std::endl;
                }
                for( unsigned int i = 0; i < obj->getExtraMetadataCount(); ++i )
                {
                    std::cout << prefix << "Extrametadata is : "
                              << obj->getExtraMetadataKeyAtIndex( i ) << ":"
                              << obj->getExtraMetadataStringValueAtIndex( i )
                              << std::endl;
                }
            }
            else
            {
                std::cout << prefix << "IS PARTIAL!" << std::endl;
            }
            std::cout << prefix
                      << "--------------------------------------------------"
                      << std::endl;
        }
    } // anonymous namespace

    /******************************************************************************/

    class GeoModelBuilderRESQMLImpl
    {
    public:
        GeoModelBuilderRESQMLImpl(
            GeoModelBuilderRESQML& builder, GeoModel3D& geomodel );
        ~GeoModelBuilderRESQMLImpl() = default;

        bool load_file();

        void deserialize( COMMON_NS::EpcDocument& pck );
        bool read_surfaces( const COMMON_NS::EpcDocument& pck );
        bool read_volumes( const COMMON_NS::EpcDocument& pck );

    private:
        GeoModelBuilderRESQML& builder_;
        GeoModel3D& geomodel_;
    };

    GeoModelBuilderRESQMLImpl::GeoModelBuilderRESQMLImpl(
        GeoModelBuilderRESQML& builder, GeoModel3D& geomodel )
        : builder_( builder ), geomodel_( geomodel )
    {
    }

    void GeoModelBuilderRESQMLImpl::deserialize( COMMON_NS::EpcDocument& pck )
    {
        std::string resqmlResult = pck.deserialize();
        if( !resqmlResult.empty() )
        {
            std::cerr << resqmlResult << std::endl;
            std::cout << "Press enter to continue..." << std::endl;
            std::cin.get();
        }

        std::cout << "EpcDocument name " << pck.getName() << " in "
                  << ( pck.getStorageDirectory().empty()
                             ? "working directory."
                             : pck.getStorageDirectory() )
                  << std::endl;

        unsigned int hdfProxyCount = pck.getHdfProxyCount();
        std::cout << "There are " << pck.getHdfProxyCount()
                  << " hdf files associated to this epc document." << std::endl;
        for( unsigned int hdfProxyIndex = 0; hdfProxyIndex < hdfProxyCount;
             ++hdfProxyIndex )
        {
            std::cout << "Hdf file relative path : "
                      << pck.getHdfProxy( hdfProxyIndex )->getRelativePath()
                      << std::endl;
        }
        for( size_t warningIndex = 0; warningIndex < pck.getWarnings().size();
             ++warningIndex )
        {
            std::cout << "Warning #" << warningIndex << " : "
                      << pck.getWarnings()[warningIndex] << std::endl;
        }
    }

    bool GeoModelBuilderRESQMLImpl::read_surfaces(
        const COMMON_NS::EpcDocument& pck )
    {
        std::vector< TriangulatedSetRepresentation* > all_tri_set_rep =
            pck.getAllTriangulatedSetRepSet();
        ringmesh_assert( !all_tri_set_rep.empty() );

        std::cout << std::endl
                  << "ALL TRI REP: " << all_tri_set_rep.size() << std::endl;

        for( size_t rep = 0; rep < all_tri_set_rep.size(); ++rep )
        {
            TriangulatedSetRepresentation* tri_set = all_tri_set_rep[rep];
            showAllMetadata( tri_set );

            const gmge_id interface_id =
                builder_.geology.create_geological_entity(
                    Interface3D::type_name_static() );

            const unsigned int patch_count = tri_set->getPatchCount();

            unsigned int global_point_count = 0;
            for( unsigned int patch = 0; patch < patch_count; ++patch )
            {
                ULONG64 pointCount = tri_set->getXyzPointCountOfPatch( patch );

                std::cout << "point Count " << pointCount << std::endl;

                std::cout << "TRI REP GEOMETRY" << std::endl;
                std::unique_ptr< double[] > xyzPoints(
                    new double[pointCount * 3] );
                tri_set->getXyzPointsOfPatch( patch, &xyzPoints[0] );

                std::vector< vec3 > points( pointCount, vec3() );
                for( unsigned int i = 0; i < pointCount; ++i )
                {
                    points[i] = vec3( xyzPoints[i * 3], xyzPoints[i * 3 + 1],
                        xyzPoints[i * 3 + 2] );
                }

                unsigned int triangleCount =
                    tri_set->getTriangleCountOfPatch( patch );
                std::cout << "triangle Count " << triangleCount << std::endl;

                std::vector< index_t > trgls( triangleCount * 3, 0 );

                tri_set->getTriangleNodeIndicesOfPatch( patch, &trgls[0] );
                for( auto& node : trgls )
                {
                    node -= global_point_count;
                }

                std::vector< index_t > trgls_ptr( triangleCount + 1, 0 );
                for( unsigned int i = 0; i < trgls_ptr.size(); ++i )
                {
                    trgls_ptr[i] = i * 3;
                }

                gmme_id children = builder_.topology.create_mesh_entity(
                    Surface3D::type_name_static() );

                builder_.geology.add_parent_children_relation(
                    interface_id, children );

                builder_.geometry.set_surface_geometry(
                    children.index(), points, trgls, trgls_ptr );

                global_point_count += pointCount;
            }
        }

        builder_.build_lines_and_corners_from_surfaces();
        builder_.build_regions_from_lines_and_surfaces();
        builder_.geology.build_contacts();
    }

    namespace
    {
        bool read_volume_rep( VolumeMesh3D& mesh,
            UnstructuredGridRepresentation& unstructed_grid )
        {
            auto mesh_builder = VolumeMeshBuilder3D::create_builder( mesh );

            unstructed_grid.loadGeometry();

            std::unique_ptr< double[] > gridPoints(
                new double[unstructed_grid.getXyzPointCountOfPatch( 0 ) * 3] );
            unstructed_grid.getXyzPointsOfAllPatchesInGlobalCrs(
                &gridPoints[0] );

            const ULONG64 nb_vertices =
                unstructed_grid.getXyzPointCountOfPatch( 0 );

            for( auto v : range( nb_vertices ) )
            {
                const vec3 vertex( gridPoints[v * 3], gridPoints[v * 3 + 1],
                    gridPoints[v * 3 + 2] );

                mesh_builder->create_vertex( vertex );
            }

            const ULONG64 nb_cells = unstructed_grid.getCellCount();
            mesh_builder->create_cells( nb_cells, CellType::TETRAHEDRON );

            std::unique_ptr< ULONG64[] > faceCountOfCells(
                new ULONG64[nb_cells] );
            unstructed_grid.getCumulativeFaceCountPerCell(
                &faceCountOfCells[0] );

            const ULONG64 faceCount = faceCountOfCells[nb_cells - 1];

            std::unique_ptr< ULONG64[] > nodeCountOfFaces(
                new ULONG64[faceCount] );
            unstructed_grid.getCumulativeNodeCountPerFace(
                &nodeCountOfFaces[0] );

            for( auto cell : range( nb_cells ) )
            {
                ULONG64 end_face = faceCountOfCells[cell];
                ULONG64 start_face =
                    ( cell == 0 ) ? 0 : faceCountOfCells[cell - 1];

                std::vector< index_t > vertices = {
                    unstructed_grid.getNodeIndicesOfFaceOfCell( cell, 0 )[0],
                    unstructed_grid.getNodeIndicesOfFaceOfCell( cell, 0 )[1],
                    unstructed_grid.getNodeIndicesOfFaceOfCell( cell, 0 )[2], 0
                };

                bool found = false;
                for( ULONG64 f = 1; f < ( end_face - start_face ); ++f )
                {
                    ULONG64 nb_nodes =
                        unstructed_grid.getNodeCountOfFaceOfCell( cell, f );

                    for( ULONG64 node = 0; node < nb_nodes; ++node )
                    {
                        const ULONG64 node_index =
                            unstructed_grid.getNodeIndicesOfFaceOfCell(
                                cell, f )[node];
                        if( node_index != vertices[0]
                            && node_index != vertices[1]
                            && node_index != vertices[2] )
                        {
                            vertices[3] = node_index;
                            found = true;
                            break;
                        }

                        if( found )
                        {
                            break;
                        }
                    }
                }
                ringmesh_assert( found );

                for( auto v_id : range( 4 ) )
                {
                    mesh_builder->set_cell_vertex(
                        { cell, v_id }, vertices[v_id] );
                }
            }
            unstructed_grid.unloadGeometry();
            mesh_builder->connect_cells();
            return true;
        }
    }

    bool GeoModelBuilderRESQMLImpl::read_volumes(
        const COMMON_NS::EpcDocument& pck )
    {
        std::vector< UnstructuredGridRepresentation* > unstructuredGridRepSet =
            pck.getUnstructuredGridRepresentationSet();
        if( unstructuredGridRepSet.empty() )
        {
            return true;
        }

        std::cout << std::endl
                  << "UNSTRUCTURED GRID REP: " << unstructuredGridRepSet.size()
                  << std::endl;

        for( auto unstructured_grid : unstructuredGridRepSet )
        {
            showAllMetadata( unstructured_grid );

            if( !unstructured_grid->isPartial()
                && unstructured_grid->hasGeometry() )
            {
                auto mesh = VolumeMesh3D::create_mesh();
                ringmesh_assert( read_volume_rep( *mesh, *unstructured_grid ) );

                // the volume mesh from resqml is here, need to find the
                // corresponding region of in the GeoModel3D
                const auto& nn_search = mesh->cell_facet_nn_search();

                int region_index = -1;
                for( auto r : range( geomodel_.nb_regions() ) )
                {
                    const Region3D& region = geomodel_.region( r );

                    bool match = true;
                    for( auto b : range( region.nb_boundaries() ) )
                    {
                        const Surface3D& surface = region.boundary( b );
                        for( auto p : range( surface.nb_mesh_elements() ) )
                        {
                            auto center = surface.mesh_element_barycenter( p );
                            auto result = nn_search.get_neighbors(
                                center, surface.geomodel().epsilon() );
                            if( result.empty() )
                            {
                                match = false;
                                break;
                            }
                        }
                        if( !match )
                        {
                            break;
                        }
                    }
                    if( match )
                    {
                        region_index = r;
                        break;
                    }
                }

                if( region_index == -1 )
                {
                    return false;
                }

                // corresponding region found, build its volume mesh
                const gmme_id region_id(
                    region_type_name_static(), (index_t) region_index );

                auto mesh_builder = builder_.geometry.create_region_builder(
                    region_id.index() );

                for( auto v : range( mesh->nb_vertices() ) )
                {
                    mesh_builder->create_vertex( mesh->vertex( v ) );
                }

                mesh_builder->create_cells(
                    mesh->nb_cells(), CellType::TETRAHEDRON );

                for( auto cell : range( mesh->nb_cells() ) )
                {
                    const index_t nb_vertices = mesh->nb_cell_vertices( cell );
                    std::vector< index_t > cell_vertices( nb_vertices, 0 );
                    for( auto v : range( nb_vertices ) )
                    {
                        ElementLocalVertex lv( cell, v );
                        mesh_builder->set_cell_vertex(
                            lv, mesh->cell_vertex( lv ) );
                    }
                }

                mesh_builder->connect_cells();
            }
        }
    }

    bool GeoModelBuilderRESQMLImpl::load_file()
    {
        COMMON_NS::EpcDocument pck(
            builder_.filename(), COMMON_NS::EpcDocument::READ_ONLY );

        deserialize( pck );

        read_surfaces( pck );

        read_volumes( pck );

        return true;
    }

    /*****************************************************************************/

    GeoModelBuilderRESQML::GeoModelBuilderRESQML(
        GeoModel3D& geomodel, const std::string& filename )
        : GeoModelBuilderFile( geomodel, std::move( filename ) ),
          impl_( new GeoModelBuilderRESQMLImpl( *this, this->geomodel_ ) )
    {
    }

    GeoModelBuilderRESQML::~GeoModelBuilderRESQML()
    {
        // needed due to the unique_ptr impl_
    }

    void GeoModelBuilderRESQML::load_file()
    {
        ringmesh_assert( impl_->load_file() );
    }

} // namespace RINGMesh