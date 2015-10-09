/*
* Copyright (c) 2012-2015, Association Scientifique pour la Geologie et ses Applications (ASGA)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the <organization> nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*  Contacts:
*     Arnaud.Botella@univ-lorraine.fr
*     Antoine.Mazuyer@univ-lorraine.fr
*     Jeanne.Pellerin@wias-berlin.de
*
*     http://www.ring-team.org
*
*     RING Project
*     Ecole Nationale Superieure de Geologie - Georessources
*     2 Rue du Doyen Marcel Roubault - TSA 70605
*     54518 VANDOEUVRE-LES-NANCY
*     FRANCE
*/

#include <ringmesh/ringmesh_tests_config.h>

#include <ringmesh/geo_model.h>
#include <ringmesh/io.h>
#include <ringmesh/geo_model_repair.h>
#include <ringmesh/geo_model_validity.h>

#include <geogram/basic/logger.h>


/*! Load and fix a given structural model file 
 */
int main( int argc, char** argv ) {
    using namespace RINGMesh ;

    GEO::Logger::out( "RINGMesh Test" ) << "Test IO for a GeoModel in .ml" << std::endl ;
    
    GeoModel M ;
    std::string file_name( ringmesh_test_data_path ) ;

    /*! @todo Make this executable generic by setting 
     *   the file name as an argument of the command */
    file_name += "annot.ml" ;

    // Set the debug directory for the validity checks 
    set_debug_directory( ringmesh_test_output_path ) ;

    // Load the model
    if( !model_load( file_name, M ) ) {
        // Try to repair the model if it is not valid
        geo_model_mesh_repair( M ) ;

        // Test the validity again
        if( is_geomodel_valid( M ) ) {
            GEO::Logger::out( "RINGMesh Test" ) << "Invalid geological model "
                << M.name() << " has been successfully fixed " << std::endl ;
            print_model( M ) ;
            return 0 ;
        } else {
            GEO::Logger::out( "RINGMesh Test" ) << "Fixing the invalid geological model "
                << M.name() << " failed. " << std::endl ;
            return 1 ;
        }
    }
    else {
        return 0 ;
    }      

}