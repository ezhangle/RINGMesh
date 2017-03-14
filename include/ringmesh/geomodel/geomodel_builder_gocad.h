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

#pragma once

#include <ringmesh/basic/common.h>

#include <geogram/basic/factory.h>
#include <geogram/basic/line_stream.h>

#include <ringmesh/geomodel/geomodel_builder.h>

namespace RINGMesh {
    class GeoModelBuilderTSolid ;
    class GeoModelBuilderML ;
    class Box3d ;
    struct VertexMap ;
    struct TSolidLoadingStorage ;
}

namespace RINGMesh {

    void RINGMESH_API initialize_gocad_import_factories() ;

    class RINGMESH_API GeoModelBuilderGocad: public GeoModelBuilderFile {
    public:
        GeoModelBuilderGocad( GeoModel& geomodel, const std::string& filename )
            : GeoModelBuilderFile( geomodel, filename ), file_line_( filename )
        {
            /*! @todo Review: A constructor is not supposed to throw, the object is left in an
             * undefined state [JP] */
            if( !file_line_.OK() ) {
                throw RINGMeshException( "I/O", "Failed to open file " + filename ) ;
            }
        }

        void build_contacts() ;

        /*!
         * @brief Parses the file and loads the GeoModel
         * @details The GeoModel loaded by this function is not valid because
         * some computation are still not done (i.e., surface internal borders,
         * lines and corners computation, boundary links between region and
         * surface, contacts)
         */
        void read_file() ;

    protected:
        virtual void read_line() = 0 ;

    protected:
        GEO::LineInput file_line_ ;
    } ;

    class GocadBaseParser: public GEO::Counted {
    ringmesh_disable_copy(GocadBaseParser) ;
    protected:
        GocadBaseParser()
            : Counted(), builder_( nil ), geomodel_( nil )
        {
        }
        virtual ~GocadBaseParser()
        {
        }

        GeoModelBuilderGocad& builder()
        {
            ringmesh_assert( builder_ != nil ) ;
            return *builder_ ;
        }

        GeoModel& geomodel()
        {
            ringmesh_assert( geomodel_ != nil ) ;
            return *geomodel_ ;
        }

        virtual void set_builder( GeoModelBuilderGocad& builder )
        {
            builder_ = &builder ;
        }

        virtual void set_geomodel( GeoModel& geomodel )
        {
            geomodel_ = &geomodel ;
        }

    private:
        GeoModelBuilderGocad* builder_ ;
        GeoModel* geomodel_ ;
    } ;

    struct GocadLoadingStorage {
        GocadLoadingStorage() ;

        /*!
         * @brief Ends a facet (by adding the size of list of facet corners at the
         * end of the vector)
         */
        void end_facet()
        {
            index_t nb_facet_corners =
                static_cast< index_t >( cur_surf_facet_corners_gocad_id_.size() ) ;
            cur_surf_facet_ptr_.push_back( nb_facet_corners ) ;
        }

        // The orientation of positive Z
        int z_sign_ ;

        std::vector< vec3 > vertices_ ;

        // Current interface index
        index_t cur_interface_ ;

        // Current surface index
        index_t cur_surface_ ;

        // List of facet corners for the current surface (gocad indices)
        std::vector< index_t > cur_surf_facet_corners_gocad_id_ ;

        // Starting indices (in cur_surf_facets_corner_gocad_id_) of each
        // facet of the current surface
        std::vector< index_t > cur_surf_facet_ptr_ ;
    } ;

    class GocadLineParser: public GocadBaseParser {
    ringmesh_disable_copy(GocadLineParser) ;
    public:
        GocadLineParser()
        {
        }
        static GocadLineParser* create(
            const std::string& keyword,
            GeoModelBuilderGocad& gm_builder,
            GeoModel& geomodel ) ;
        virtual void execute(
            GEO::LineInput& line,
            GocadLoadingStorage& load_storage ) = 0 ;
    } ;

    typedef GEO::SmartPointer< GocadLineParser > GocadLineParser_var ;
    typedef GEO::Factory0< GocadLineParser > GocadLineParserFactory ;
#define ringmesh_register_GocadLineParser_creator(type, name) \
     geo_register_creator(GocadLineParserFactory, type, name)

    /*!
     * @brief Structure which maps the vertex indices in Gocad::TSolid to the
     * pair (region, index in region) in the RINGMesh::GeoModel
     */
    struct VertexMap {
        VertexMap()
        {
        }

        index_t local_id( index_t gocad_vertex_id ) const
        {
            return gocad_vertices2region_vertices_[gocad_vertex_id] ;
        }

        index_t region( index_t gocad_vertex_id ) const
        {
            return gocad_vertices2region_id_[gocad_vertex_id] ;
        }

        void add_vertex( index_t local_vertex_id, index_t region_id )
        {
            gocad_vertices2region_vertices_.push_back( local_vertex_id ) ;
            gocad_vertices2region_id_.push_back( region_id ) ;
        }

        index_t nb_vertex() const
        {
            ringmesh_assert(
                gocad_vertices2region_vertices_.size()
                == gocad_vertices2region_id_.size() ) ;
            return static_cast< index_t >( gocad_vertices2region_vertices_.size() ) ;
        }

        void reserve( index_t capacity )
        {
            gocad_vertices2region_vertices_.reserve( capacity ) ;
            gocad_vertices2region_id_.reserve( capacity ) ;
        }

    private:
        /*!
         * Mapping the indices of vertices from Gocad .so file
         * to the local (in region) indices of vertices
         */
        std::vector< index_t > gocad_vertices2region_vertices_ ;
        /*!
         * Mapping the indices of vertices from Gocad .so file
         * to the region containing them
         */
        std::vector< index_t > gocad_vertices2region_id_ ;
    } ;

    /*!
     * @brief Structure used to load a GeoModel by GeoModelBuilderTSolid
     */
    struct TSolidLoadingStorage: public GocadLoadingStorage {
        TSolidLoadingStorage() ;

        // Current region index
        index_t cur_region_ ;

        // Map between gocad and GeoModel vertex indices
        VertexMap vertex_map_ ;

        // Region tetrahedron corners
        std::vector< index_t > tetra_corners_ ;

    } ;
    class TSolidLineParser: public GocadBaseParser {
    ringmesh_disable_copy(TSolidLineParser) ;
    public:
        TSolidLineParser()
        {
        }
        static TSolidLineParser* create(
            const std::string& keyword,
            GeoModelBuilderTSolid& gm_builder,
            GeoModel& geomodel ) ;
        virtual void execute(
            GEO::LineInput& line,
            TSolidLoadingStorage& load_storage ) = 0 ;
    } ;

    typedef GEO::SmartPointer< TSolidLineParser > TSolidLineParser_var ;
    typedef GEO::Factory0< TSolidLineParser > TSolidLineParserFactory ;
#define ringmesh_register_TSolidLineParser_creator(type, name) \
     geo_register_creator(TSolidLineParserFactory, type, name)

    /*!
     * @brief Builds a meshed GeoModel from a Gocad TSolid (file.so)
     */
    class RINGMESH_API GeoModelBuilderTSolid: public GeoModelBuilderGocad {
    public:
        GeoModelBuilderTSolid( GeoModel& geomodel, const std::string& filename )
            : GeoModelBuilderGocad( geomodel, filename )
        {
        }
        virtual ~GeoModelBuilderTSolid()
        {
        }

    private:
        virtual void load_file() ;

        /*!
         * @brief Reads the first word of the current line (keyword)
         * and executes the good action with the information of the line
         * @details Uses the TsolidLineParser factory
         */
        virtual void read_line() ;

        /*!
         * @brief Computes internal borders of a given surface
         * @details A surface facet edge is an internal border if it is shared
         * by at least two surfaces. Adjacency of such a facet edge is set to
         * GEO::NO_FACET.
         * @param[in] geomodel GeoModel to consider
         * @param[in] surface_id Index of the surface
         * @param[in] surface_nns Pointers to the NNSearchs of surfaces
         */
        void compute_surface_internal_borders(
            index_t surface_id,
            const std::vector< NNSearch* >& surface_nns,
            const std::vector< Box3d >& surface_boxes ) ;

        /*!
         * @brief Computes the NNSearchs of the centers of facet edges for
         * each surface and their Box3d
         * @param[in] geomodel GeoModel to consider
         * @param[out] surface_nns Pointers to the NNSearchs of surfaces
         * @param[out] surface_boxes Bounding Box of surfaces
         */
        void compute_facet_edge_centers_nn_and_surface_boxes(
            std::vector< NNSearch* >& surface_nns,
            std::vector< Box3d >& surface_boxes ) ;

        /*!
         * @brief Computes internal borders of the geomodel surfaces
         * @details An surface facet edge is an internal border if it is shared
         * by at least two surfaces. Adjacency of such a facet edge is set to
         * GEO::NO_FACET.
         * @param[in] geomodel GeoModel to consider
         */
        void compute_surfaces_internal_borders() ;

    private:
        TSolidLoadingStorage tsolid_load_storage_ ;
        friend class RINGMesh::GocadLineParser ;
    } ;

    struct MLLoadingStorage: public GocadLoadingStorage {
        MLLoadingStorage() ;

        bool is_header_read_ ;

        /// Offset to read in the tface vertices in the tsurf vertices
        index_t tface_vertex_ptr_ ;
    } ;
    class MLLineParser: public GocadBaseParser {
    ringmesh_disable_copy(MLLineParser) ;
    public:
        MLLineParser()
        {
        }
        static MLLineParser* create(
            const std::string& keyword,
            GeoModelBuilderML& gm_builder,
            GeoModel& geomodel ) ;
        virtual void execute(
            GEO::LineInput& line,
            MLLoadingStorage& load_storage ) = 0 ;
    } ;

    typedef GEO::SmartPointer< MLLineParser > MLLineParser_var ;
    typedef GEO::Factory0< MLLineParser > MLLineParserFactory ;
#define ringmesh_register_MLLineParser_creator(type, name) \
     geo_register_creator(MLLineParserFactory, type, name)

    /*!
     * @brief Build a GeoModel from a Gocad Model3D (file_model.ml)
     */
    class RINGMESH_API GeoModelBuilderML: public GeoModelBuilderGocad {
    public:
        GeoModelBuilderML( GeoModel& geomodel, const std::string& filename )
            : GeoModelBuilderGocad( geomodel, filename )
        {
        }
        virtual ~GeoModelBuilderML()
        {
        }

    private:
        void load_file() ;

        /*!
         * @brief Reads the first word of the current line (keyword)
         * and executes the good action with the information of the line
         * @details Uses the MLLineParser factory
         */
        virtual void read_line() ;

    private:
        MLLoadingStorage ml_load_storage_ ;
    } ;

}