/*
*   Description:
*
*      HOOPS Exchange Polygonica Bridge
*      Functions provided to facilitate loading files 
*      into Polygonica types using HOOPS Exchange
* 
*      Use the following functions to initialise and terminate the library:
*      A3DSDKLoadLibrary, A3DDllIsInitialized, A3DLicPutUnifiedLicense, A3DDllInitialize, A3DDllTerminate
*      Use the following to load a model file:
*      A3DRWParamsLoadData, A3D_INITIALIZE_DATA, A3DAsmModelFileLoadFromFile, A3DAsmModelFileDelete
*      Use the following to create Polygonica data from the model file:
*      A3DModelCreatePGWorld, A3DDestroyBridgeWorldEntities, A3DDestroyBridgeSolids, A3DDestroyBridgeData
*      The Polygonica data are available in the A3DPolygonicaOptions struct
*/
#pragma once

//===includes==========================================================

#include <A3DSDKIncludes.h>
#include "pg/pgapi.h"
#ifdef TEXTURE_FORMAT_SUPPORT
// Handling PNG and JPEG textures requires conversion to RGB
// Support for this is not provided by HOOPS Exchange / Polygonica
#include "pg_texture_util.h"
#endif

#include <algorithm>
#include <unordered_map>
#include <map>
#include <string>

//===defines===========================================================

#define A3D_PG_NOT_INITIALIZED   1
#define A3D_PG_INVALID_RI        2
#define A3D_PG_ERROR             3

// The use of 'static' is to provide support for older compilers that do not support 'inline'
// If you are using a modern compiler inline should probably be used
#ifdef __cplusplus
#define INTERNAL inline
#else
#define INTERNAL static
#endif

#define COPY(dest, src, size_)             \
   if (size_ != 0)                         \
   {                                       \
      unsigned uTempSize = dest.size();    \
      dest.resize(uTempSize + size_);      \
      std::copy(src, src+ size_, dest.begin() + uTempSize); \
      src+= size_;                         \
      size_ = 0;                           \
   }

#define COPY_(dest, src, size_)            \
   if (size_)                              \
   {                                       \
      size_t uTempSize = dest.size();      \
      dest.resize(uTempSize + size_);      \
      std::copy(src, src+size_, dest.begin() + uTempSize); \
   }

//===structs===========================================================

enum A3D_log_level
{
   A3D_LOG_INFO,
   A3D_LOG_WARN,
   A3D_LOG_ERROR
};
//---A3D_log_level-----------------------------------------------------

typedef void (*A3D_log_func)(std::string, A3D_log_level);

struct A3DPolygonicaOptions
{
   PTEnvironment m_Environment;
   PTWorld m_World;

   // A map relating each A3DRiRepresentationItem to a PTSolid
   std::unordered_map<const A3DRiRepresentationItem*, PTSolid> m_parts;

   // A vector of PTWorldEntities
   std::vector<PTWorldEntity> m_entities;
   // A map relating each PTWorldEntity to a vector part path
   std::unordered_map<PTWorldEntity, std::vector<void*>*> m_paths;
   // A map relating a PTWorldEntity to an RGB double colour
   // as an RGB double array
   std::unordered_map<PTWorldEntity, A3DDouble*> m_entity_colours;

   // A map relating each PTAppSurface to a CAD face
   std::map<PTAppSurface, A3DTopoFace*> m_surface_faces;
   // A map relating each PTAppSurface to an orientation with respect to shell (of the CAD face)
   std::map<PTAppSurface, A3DUns8> m_surface_face_orientations;
   // A map relating each PTAppSurface (CAD face) to a group of PTFaces (polygons)
   std::map<PTAppSurface, PTEntityGroup> m_surface_groups;
   // A map relating a PTAppSurface to a texture index
   // These indices allow access to the picture data of the texture through m_texture_palette
   std::map<PTAppSurface, A3DUns32> m_surface_textures;
   // A map relating a PTAppSurface to a colour
   std::map<PTAppSurface, A3DDouble*> m_surface_colours;
   long m_iTopoFaceCount = 0;

   // A map providing one A3DGraphPictureData for each texture definition index
   std::map<A3DUns32, A3DGraphPictureData*> m_texture_palette;

   // A vector of all A3DRiBrepModels in the model
   // as this may be required to call A3DProjectPointCloud.
   std::vector<A3DRiBrepModel*> m_breps;

   // Scale (relative to mm)
   // This is required to convert to m when querying points using A3DTopoFaceQuery.
   // The scale represents the native scale of the underlying modelling system 
   // which the CAD system used which generated the CAD file. 
   // Some work in mm others metres. 
   // NB Model unit (1.0 for mm, 25.4 for inches) is handled by world entity transform 
   double m_scale = 1.0;
};
//---A3DPolygonicaOptions----------------------------------------------

struct MiscCascadedAttributesGuard
{
   A3DMiscCascadedAttributes* ptr_;

   MiscCascadedAttributesGuard(A3DMiscCascadedAttributes* ptr) : ptr_(ptr)
   {}

   ~MiscCascadedAttributesGuard()
   {
      if (A3DMiscCascadedAttributesDelete(ptr_))
      {
         ptr_ = NULL;
      }
   }
};
//---MiscCascadedAttributesGuard---------------------------------------

// Used in get_solid_mesh_data
typedef struct _PolygonList
{
   // For each surface/CAD face, store 
   // a list of indices into the points array (3 per face) 
   // a list of normals (3x3 doubles per face)
   // a Boolean to indicate if the surface had a colour associated with it
   // an RGB double colour
   std::vector<PTNat32>indices;
   std::vector<double>normals;
   bool has_colour;
   double colour[3];
} PolygonList;
//---PolygonList-------------------------------------------------------

// Used in get_solid_mesh_data
typedef struct _PGMeshCallbackData
{
   PTNat32  num_points;
   PTPoint* points; // num_points
   PTNat32  num_triangles;
   PTNat32* indices; // One for each vertex = num_triangles * 3
   PTPoint* normals; // One for each vertex = num_triangles * 3
   PTNat32  num_surfaces;
   PTNat32* polygons_per_surface; // num_surfaces (one for each unique PTAppSurface)
   double** surface_colours; // num_surfaces

   // Map used to store indices and normals for each surface separately 
   // allowing indices and normals to be sorted by PTAppSurface in the pg_mesh_end_callback
   std::map<PTAppSurface, PolygonList> surfaces;
} PGMeshCallbackData;
//---PGMeshCallbackData------------------------------------------------

//===functions=========================================================

// If using Polygonica rendering, enable colour-handling for world entities
#ifdef PGRENDER_HDR
INTERNAL PTStatus set_render_style_from_colour(PTEnvironment env,
                                               PTWorldEntity world_entity, 
                                               double*       colour)
{
   // Set the render style of the world entity
   // from an RGB float colour
   PTStatus       status = PV_STATUS_OK;
   PTRenderStyle  render_style;
   PTPolygonStyle polygon_style;

   // Retrieve the existing render style or create a new one
   render_style = PFEntityGetEntityProperty(world_entity, PV_WENTITY_PROP_STYLE);
   if (render_style == PV_ENTITY_NULL)
   {
      PFRenderStyleCreate(env, &render_style);
   }

   // Set the polygon render style with the colour
   polygon_style = PFEntityGetEntityProperty(render_style, PV_RSTYLE_PROP_POLYGON_STYLE);
   PFEntitySetColourProperty(polygon_style, PV_PSTYLE_PROP_COLOUR, PV_COLOUR_DOUBLE_RGB_ARRAY, colour);
   PFEntitySetColourProperty(polygon_style, PV_PSTYLE_PROP_BACK_COLOUR, PV_COLOUR_DOUBLE_RGB_ARRAY, colour);
   PFEntitySetNat32Property(polygon_style, PV_PSTYLE_PROP_TRANSPARENCY, 0);
   // No edge rendering
   PFEntitySetEntityProperty(render_style, PV_RSTYLE_PROP_EDGE_STYLE, 0);

   PFEntitySetEntityProperty(world_entity, PV_WENTITY_PROP_STYLE, render_style);

   return status;
}
//---set_render_style_from_colour--------------------------------------

INTERNAL PTBoolean get_colour_from_render_style(PTWorldEntity world_entity, double* colour)
{
   // Get the polygon colour of the world entity as an array of RGB doubles
   // Return TRUE if the world entity has a colour, FALSE otherwise
   PTStatus       status = PV_STATUS_OK;
   PTRenderStyle  render_style;
   PTPolygonStyle polygon_style;
   double* polygon_colour;

   if (world_entity == PV_ENTITY_NULL)
   {
      return FALSE;
   }

   render_style = PFEntityGetEntityProperty(world_entity, PV_WENTITY_PROP_STYLE);
   polygon_style = PFEntityGetEntityProperty(render_style, PV_RSTYLE_PROP_POLYGON_STYLE);
   polygon_colour = (double*)PFEntityGetColourProperty(polygon_style, PV_PSTYLE_PROP_COLOUR, PV_COLOUR_DOUBLE_RGB_ARRAY);
   if (polygon_colour == NULL)
   {
      return FALSE;
   }

   colour[0] = polygon_colour[0];
   colour[1] = polygon_colour[1];
   colour[2] = polygon_colour[2];

   return TRUE;
}
//---get_colour_from_render_style--------------------------------------

INTERNAL int create_pg_texture(A3DGraphPictureData* pPictureData,
                               PTEnvironment env,
                               PTTexture* texture_out)
{
   PTStatus status = PV_STATUS_OK;
   if ((pPictureData->m_eFormat != kA3DPictureBitmapRgbByte) &&
       (pPictureData->m_eFormat != kA3DPictureBitmapRgbaByte))
   {
#ifdef TEXTURE_FORMAT_SUPPORT
      // Convert from PNG/JPEG to RGB data and create PTTexture
      if (pPictureData->m_eFormat == kA3DPictureJpg)
      {
         *texture_out = PGTextureFromJPEGData(env,
                                              (unsigned char*)pPictureData->m_pucBinaryData,
                                              (size_t)pPictureData->m_uiSize);
      }
      else if (pPictureData->m_eFormat == kA3DPicturePng)
      {
         *texture_out = PGTextureFromPNGData(env,
                                             (unsigned char*)pPictureData->m_pucBinaryData,
                                             (size_t)pPictureData->m_uiSize);
      }
      else
#endif
      {
         return PV_STATUS_BAD_CALL;
      }
   }
   else
   {
      // Create a PTTexture from the picture data
      status = PFTextureCreate(env,
                               (PTNat32)pPictureData->m_uiPixelWidth,
                               (PTNat32)pPictureData->m_uiPixelHeight,
                               PV_TEXTURE_TYPE_RGB,
                               (pPictureData->m_eFormat == kA3DPictureBitmapRgbByte) ? PV_TEXTURE_FORMAT_NAT8_RGB : PV_TEXTURE_FORMAT_NAT8_RGBA,
                               (void*)pPictureData->m_pucBinaryData,
                               texture_out);
   }

   return status;
}
//---create_pg_texture-------------------------------------------------

INTERNAL PTStatus set_pg_textures(A3DPolygonicaOptions& bridge_data, 
                                  std::vector<PTTexture>& textures)
{
   // Create PTTextures from the texture palette in bridge_data
   // and set these on PTFaces of associated PTAppSurfaces
   // Return a vector of the created PTTextures
   PTStatus       status = PV_STATUS_OK;
   PTAppSurface   surface = PV_ENTITY_NULL;
   A3DUns32       texture_index;
   A3DGraphPictureData* texture_data;
   PTTexture      texture = PV_ENTITY_NULL;
   PTEntityGroup  surface_group = PV_ENTITY_NULL;
   PTEntityList   polygons = PV_ENTITY_NULL;
   PTFace         polygon = PV_ENTITY_NULL;

   // For each PTAppSurface with a texture index
   for (auto it = bridge_data.m_surface_textures.begin();
       it != bridge_data.m_surface_textures.end();
       it++)
   {
      surface = (PTAppSurface)it->first;
      texture_index = (A3DUns32)it->second;

      // Retrieve the original texture picture data from the texture palette
      texture_data = (A3DGraphPictureData*)bridge_data.m_texture_palette[texture_index];
      if (texture_data == NULL)
      {
         continue;
      }

      // Create a PTTexture
      status = create_pg_texture(texture_data, bridge_data.m_Environment, &texture);
      if (status != PV_STATUS_OK)
      {
         return status;
      }

      // Add the PTTexture to the output list
      textures.push_back(texture);

      // Set the PTTexture on all PTFaces associated with the PTAppSurface
      auto search = bridge_data.m_surface_groups.find(surface);
      if (search == bridge_data.m_surface_groups.end())
         //if (!bridge_data->m_surface_groups.contains(app_surface))
      {
         continue;
      }
      //surface_group = (PTEntityGroup)bridge_data->m_surface_groups[app_surface];
      surface_group = (PTEntityGroup)search->second;

      PFEntityCreateEntityList(surface_group, PV_ENTITY_TYPE_FACE, NULL, &polygons);
      for (polygon = PFEntityListGetFirst(polygons);
         polygon != PV_ENTITY_NULL;
         polygon = PFEntityListGetNext(polygons, polygon))
      {
         PFEntitySetEntityProperty(polygon, PV_FACE_PROP_TEXTURE, texture);
      }
      PFEntityListDestroy(polygons, 0);
   }

   return status;
}
//---set_pg_textures---------------------------------------------------
#endif

INTERNAL PTStatus set_pg_surface_colours(A3DPolygonicaOptions& bridge_data)
{
   // Set colours on PTFaces of associated PTAppSurfaces
   // using bridge_data.m_surface_colours to determine colour
   // and bridge_data.m_surface_groups to determine PTFaces associated with a PTAppSurface
   PTStatus       status = PV_STATUS_OK;
   PTAppSurface   surface = PV_ENTITY_NULL;
   double* colour = NULL;
   PTEntityGroup  surface_group = PV_ENTITY_NULL;
   PTEntityList   polygons = PV_ENTITY_NULL;
   PTFace         polygon = PV_ENTITY_NULL;

   // For each PTAppSurface with a texture index
   for (auto it = bridge_data.m_surface_colours.begin();
        it != bridge_data.m_surface_colours.end();
        it++)
   {
      surface = (PTAppSurface)it->first;
      colour = (double*)it->second;

      // Set the colour on all PTFaces associated with the PTAppSurface
      auto search = bridge_data.m_surface_groups.find(surface);
      if (search == bridge_data.m_surface_groups.end())
      {
         continue;
      }
      surface_group = (PTEntityGroup)search->second;

      PFEntityCreateEntityList(surface_group, PV_ENTITY_TYPE_FACE, NULL, &polygons);
      for (polygon = PFEntityListGetFirst(polygons);
           polygon != PV_ENTITY_NULL;
           polygon = PFEntityListGetNext(polygons, polygon))
      {
         PFEntitySetColourProperty(polygon, PV_FACE_PROP_COLOUR,
                                   PV_COLOUR_DOUBLE_RGB_ARRAY, colour);
      }
      PFEntityListDestroy(polygons, 0);
   }

   return status;
}
//---set_pg_surface_colours--------------------------------------------

INTERNAL PTBoolean get_colour_from_map(PTWorldEntity world_entity,
                                       std::unordered_map<PTWorldEntity, A3DDouble*>& colour_map,
                                       double* colour)
{
   // Get the colour associated with a world entity in a colour map
   // Return TRUE if the world entity has a colour, FALSE otherwise
   double* mapped_colour = NULL;
   if (world_entity == PV_ENTITY_NULL)
   {
      return FALSE;
   }

   auto search = colour_map.find(world_entity);
   if (search == colour_map.end())
   {
      return FALSE;
   }

   mapped_colour = (double*)search->second;
   if (mapped_colour == NULL)
   {
      return FALSE;
   }

   colour[0] = mapped_colour[0];
   colour[1] = mapped_colour[1];
   colour[2] = mapped_colour[2];

   return TRUE;
}
//---get_colour_from_map-----------------------------------------------

INTERNAL void CHECK_A3DSTATUS(A3DStatus status,
                              A3D_log_func logger, 
                              const char* operation_name)
{
   if (logger == nullptr || status == A3D_SUCCESS)
   {
      return;
   }
   auto error_code_message = std::string(A3DMiscGetErrorMsg(status));
   logger(error_code_message + " ( in: " + operation_name + ")", A3D_log_level::A3D_LOG_ERROR);
}
//---CHECK_A3DSTATUS---------------------------------------------------

INTERNAL void CHECK_PTSTATUS(PTStatus status,
                             A3D_log_func logger, 
                             const char* operation_name)
{
   if (logger == nullptr || status == PV_STATUS_OK)
   {
      return;
   }
   logger("Polygonica error " + std::to_string(status) + " ( in: " + operation_name + ")", A3D_log_level::A3D_LOG_ERROR);
}
//---CHECK_PTSTATUS----------------------------------------------------

INTERNAL void log(A3D_log_func logging_function,
                  std::string message, A3D_log_level level)
{
   if (logging_function == nullptr)
   {
      return;
   }
   logging_function(message, level);
}
//---log---------------------------------------------------------------

INTERNAL int A3DDestroyBridgeSolids(A3DPolygonicaOptions& bridge_data)
{
   // Destroy PTSolids created by the bridge
   for (auto i = bridge_data.m_parts.begin();
        i != bridge_data.m_parts.end(); i++)
   {
      PFSolidDestroy((PTSolid)i->second);
   }
   return A3D_SUCCESS;
}
//---A3DDestroyBridgeSolids--------------------------------------------

INTERNAL int A3DDestroyBridgeWorldEntities(A3DPolygonicaOptions& bridge_data)
{
   // Destroy PTWorldEntities created by the bridge
   for (auto i = bridge_data.m_entities.begin();
        i != bridge_data.m_entities.end(); i++)
   {
      PFWorldRemoveEntity((PTWorldEntity)*i);
   }
   return A3D_SUCCESS;
}
//---A3DDestroyBridgeWorldEntities-------------------------------------

INTERNAL int A3DDestroyBridgePartsData(A3DPolygonicaOptions& bridge_data)
{
   // Destroy parts data created by the bridge in the A3DPolygonicaOptions struct
   // NB This does not destroy the PTSolids
   bridge_data.m_parts.clear();
   return A3D_SUCCESS;
}
//---A3DDestroyBridgePartsData-----------------------------------------

INTERNAL int A3DDestroyBridgeEntitiesData(A3DPolygonicaOptions& bridge_data)
{
   // Destroy vector of entities created by the bridge in the A3DPolygonicaOptions struct
   // NB This does not destroy the PTWorldEntities
   bridge_data.m_entities.clear();

   // Destroy path data created by the bridge in the A3DPolygonicaOptions struct
   for (auto i = bridge_data.m_paths.begin();
      i != bridge_data.m_paths.end(); i++)
   {
      std::vector<void*>* path = (std::vector<void*>*)i->second;
      delete path;
   }
   bridge_data.m_paths.clear();

   // Destroy world entity colours created by the bridge in the A3DPolygonicaOptions struct
   for (auto i = bridge_data.m_entity_colours.begin();
      i != bridge_data.m_entity_colours.end(); i++)
   {
      if (i->second != NULL)
      {
         delete[] i->second;
      }
   }
   bridge_data.m_entity_colours.clear();

   return A3D_SUCCESS;
}
//---A3DDestroyBridgeEntitiesData--------------------------------------

INTERNAL int A3DDestroyBridgeSurfacesData(A3DPolygonicaOptions& bridge_data)
{
   // Destroy data relating to PTAppSurfaces and associated maps 
   // created by the bridge in the A3DPolygonicaOptions struct

   // Destroy surface groups
   for (auto i = bridge_data.m_surface_groups.begin();
      i != bridge_data.m_surface_groups.end(); i++)
   {
      PTEntityGroup surface_group = (PTEntityGroup)i->second;
      PFEntityGroupDestroy(surface_group);
   }
   bridge_data.m_surface_groups.clear();

   // Destroy surface colours
   for (auto i = bridge_data.m_surface_colours.begin();
        i != bridge_data.m_surface_colours.end(); i++)
   {
      if (i->second != NULL)
      {
         delete[] i->second;
      }
   }
   bridge_data.m_surface_colours.clear();

   // Clear CAD faces and orientations
   bridge_data.m_surface_faces.clear();
   bridge_data.m_surface_face_orientations.clear();

   return A3D_SUCCESS;
}
//---A3DDestroyBridgeSurfacesData--------------------------------------

INTERNAL int A3DDestroyBridgeTexturesData(A3DPolygonicaOptions& bridge_data)
{
   // Destroy textures created by the bridge in the A3DPolygonicaOptions struct
   for (auto i = bridge_data.m_texture_palette.begin();
        i != bridge_data.m_texture_palette.end(); i++)
   {
      A3DGlobalGetGraphPictureData(A3D_DEFAULT_PICTURE_INDEX, 
                                   (A3DGraphPictureData*)i->second);
   }
   bridge_data.m_texture_palette.clear();
   return A3D_SUCCESS;
}
//---A3DDestroyBridgeTexturesData--------------------------------------

INTERNAL int A3DDestroyBridgeBrepsData(A3DPolygonicaOptions& bridge_data)
{
   // Clear BReps vector created by the bridge in the A3DPolygonicaOptions struct
   bridge_data.m_breps.clear();
   return A3D_SUCCESS;
}
//---A3DDestroyBridgePartsData-----------------------------------------

INTERNAL int A3DDestroyBridgeData(A3DPolygonicaOptions& bridge_data)
{
   // Destroy data created by the bridge in the A3DPolygonicaOptions struct
   // NB This does not destroy the PTSolids or PTWorldEntities
   // Use A3DDestroyBridgeSolids and A3DDestroyBridgeWorldEntities if this is required

   A3DDestroyBridgePartsData(bridge_data);
   A3DDestroyBridgeEntitiesData(bridge_data);
   A3DDestroyBridgeSurfacesData(bridge_data);
   A3DDestroyBridgeTexturesData(bridge_data);
   A3DDestroyBridgeBrepsData(bridge_data);

   bridge_data.m_iTopoFaceCount = 0;
   bridge_data.m_scale = 1.0;

   return A3D_SUCCESS;
}
//---A3DDestroyBridgeData----------------------------------------------

/*!
\brief Returns the name of the part or product occurrence
\param pPartOrProduct The part or product occurrence
\param name [out] The name
\return A3D_SUCCESS - Operation succeeded
*/
INTERNAL A3DStatus A3DGetName(const A3DRootBaseWithGraphics* pPartOrProduct,
                              std::string& name, A3D_log_func logging_function)
{
   A3DRootBaseData sRootBaseData;
   A3D_INITIALIZE_DATA(A3DRootBaseData, sRootBaseData);
   CHECK_A3DSTATUS(A3DRootBaseGet(pPartOrProduct, &sRootBaseData), 
                   logging_function, "stGetName - A3DRootBaseGet");
   //const DATAGUARD(A3DRootBase) sGuard(sRootBaseData, A3DRootBaseGet);

   if (sRootBaseData.m_pcName != NULL)
   {
      name = std::string(sRootBaseData.m_pcName);
   }

   A3DRootBaseGet(NULL, &sRootBaseData);

   return A3D_SUCCESS;
}
//---A3DGetName--------------------------------------------------------

INTERNAL A3DStatus get_color_from_graphic_data(const A3DGraphStyleData& sGraphStyleData,
                                               A3DDouble& r, A3DDouble& g, A3DDouble& b,
                                               A3D_log_func logging_function)
{
   // Gets the inherited color information
   A3DUns32 uiRgbColorIndex;

   if (sGraphStyleData.m_bMaterial == TRUE)
   {
      A3DBool isTexture = false;
      CHECK_A3DSTATUS(A3DGlobalIsMaterialTexture(sGraphStyleData.m_uiRgbColorIndex, &isTexture), 
                      logging_function, "stExtractColorFromGraphicData");

      if (isTexture)
      {
         log(logging_function, "stExtractColorFromGraphicData can't handle textured materials", A3D_LOG_ERROR);
      }
      else // Material
      {
         A3DGraphMaterialData gmd;
         A3D_INITIALIZE_DATA(A3DGraphMaterialData, gmd);
         A3DGlobalGetGraphMaterialData(sGraphStyleData.m_uiRgbColorIndex, &gmd);

         uiRgbColorIndex = gmd.m_uiDiffuse;
      }
   }
   else
   {
      uiRgbColorIndex = sGraphStyleData.m_uiRgbColorIndex;
   }

   A3DGraphRgbColorData sColorData;
   A3D_INITIALIZE_DATA(A3DGraphRgbColorData, sColorData);

   if (A3DGlobalGetGraphRgbColorData(uiRgbColorIndex, &sColorData) == A3D_SUCCESS)
   {
      r = sColorData.m_dRed;
      g = sColorData.m_dGreen;
      b = sColorData.m_dBlue;
   }

   A3DGlobalGetGraphRgbColorData(A3D_DEFAULT_COLOR_INDEX, &sColorData);

   return A3D_SUCCESS;
}
//---get_color_from_graphic_data---------------------------------------

INTERNAL A3DStatus create_and_push_cascaded_attributes(const A3DRootBaseWithGraphics* pBase,
                                                       const A3DMiscCascadedAttributes* pFatherAttr,
                                                       A3DMiscCascadedAttributes** ppAttr,
                                                       A3DMiscCascadedAttributesData* psAttrData,
                                                       A3D_log_func logging_function)
{
   // Creates an A3DMiscCascadedAttributes structure and computes the attributes
   // based on the input A3DRootBaseWithGraphics structure pBase.
   CHECK_A3DSTATUS(A3DMiscCascadedAttributesCreate(ppAttr), logging_function, 
                   "create_and_push_cascaded_attributes - A3DMiscCascadedAttributesCreate");
   CHECK_A3DSTATUS(A3DMiscCascadedAttributesPush(*ppAttr, pBase, pFatherAttr), logging_function, 
                   "create_and_push_cascaded_attributes - A3DMiscCascadedAttributesPush");

   A3D_INITIALIZE_DATA(A3DMiscCascadedAttributesData, (*psAttrData));
   CHECK_A3DSTATUS(A3DMiscCascadedAttributesGet(*ppAttr, psAttrData), logging_function, 
                   "create_and_push_cascaded_attributes - A3DMiscCascadedAttributesGet");
   return A3D_SUCCESS;
}
//---create_and_push_cascaded_attributes-------------------------------

INTERNAL A3DStatus indices_per_face_as_triangles(A3DTess3DData sTessData,
                                                 const unsigned& uFaceIndice,
                                                 std::vector<unsigned>& auIndices,
                                                 std::vector<PTInt32>& normal_indices,
                                                 std::vector<PTInt32>& texture_indices,
                                                 A3D_log_func logging_function)
{
   A3DTessFaceData* pFaceTessData = &(sTessData.m_psFaceTessData[uFaceIndice]);

   if (!pFaceTessData->m_uiSizesTriangulatedSize)
   {
      return A3D_SUCCESS;
   }

   A3DUns32* puiTriangulatedIndexes = sTessData.m_puiTriangulatedIndexes
                                      + pFaceTessData->m_uiStartTriangulated;

   unsigned uiCurrentSize = 0;
   A3DUns16  unprocessed_flags = pFaceTessData->m_usUsedEntitiesFlags;

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangle)
   {
      unprocessed_flags &= ~kA3DTessFaceDataTriangle;
      A3DUns32 uNbTriangles = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];

      for (A3DUns32 uI = 0; uI < uNbTriangles; uI++)
      {
         for (int i = 0; i < 3; i++)
         {
            COPY_(normal_indices, puiTriangulatedIndexes, 1);
            puiTriangulatedIndexes += 1; // Move past the normal
            COPY_(auIndices, puiTriangulatedIndexes, 1);
            puiTriangulatedIndexes += 1;
         }
      }
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleFan)
   {
      unprocessed_flags &= ~kA3DTessFaceDataTriangleFan;
      A3DUns32 uiNbFan = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];

      for (A3DUns32 uiFan = 0; uiFan < uiNbFan; uiFan++)
      {
         A3DUns32 uiNbPoint = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];

         A3DUns32* pFanPointNormalIndice = puiTriangulatedIndexes;
         puiTriangulatedIndexes += 1;

         A3DUns32* pFanPointIndice = puiTriangulatedIndexes;
         puiTriangulatedIndexes += 2;
         A3DUns32 uIPoint;
         for (uIPoint = 1; uIPoint < uiNbPoint - 1; uIPoint++)
         {
            COPY_(normal_indices, pFanPointNormalIndice, 1);
            COPY_(auIndices, pFanPointIndice, 1);
            COPY_(normal_indices, puiTriangulatedIndexes - 1, 1);
            COPY_(auIndices, puiTriangulatedIndexes, 1);
            COPY_(normal_indices, puiTriangulatedIndexes + 1, 1);
            COPY_(auIndices, puiTriangulatedIndexes + 2, 1);
            puiTriangulatedIndexes += 2;
         }
         puiTriangulatedIndexes += 1;
      }
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleStripe)
   {
      unprocessed_flags &= ~kA3DTessFaceDataTriangleStripe;
      A3DUns32 uiNbStripe = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];

      A3DUns32 uiStripe;
      for (uiStripe = 0; uiStripe < uiNbStripe; uiStripe++)
      {
         A3DUns32 uiNbPoint = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];
         puiTriangulatedIndexes += 3;
         A3DUns32 uIPoint;
         for (uIPoint = 0; uIPoint < uiNbPoint - 2; uIPoint++)
         {
            COPY_(normal_indices, puiTriangulatedIndexes - 1, 1);
            COPY_(auIndices, puiTriangulatedIndexes, 1);
            if (uIPoint % 2)
            {
               COPY_(normal_indices, puiTriangulatedIndexes - 3, 1);
               COPY_(auIndices, puiTriangulatedIndexes - 2, 1);
               COPY_(normal_indices, puiTriangulatedIndexes + 1, 1);
               COPY_(auIndices, puiTriangulatedIndexes + 2, 1);
            }
            else
            {
               COPY_(normal_indices, puiTriangulatedIndexes + 1, 1);
               COPY_(auIndices, puiTriangulatedIndexes + 2, 1);
               COPY_(normal_indices, puiTriangulatedIndexes - 3, 1);
               COPY_(auIndices, puiTriangulatedIndexes - 2, 1);
            }
            puiTriangulatedIndexes += 2;
         }
         puiTriangulatedIndexes += 1;
      }
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleOneNormal)
   {
      unprocessed_flags &= ~kA3DTessFaceDataTriangleOneNormal;
      A3DUns32 uNbTriangles = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];

      for (A3DUns32 uI = 0; uI < uNbTriangles; uI++)
      {
         COPY_(normal_indices, puiTriangulatedIndexes, 1);
         COPY_(normal_indices, puiTriangulatedIndexes, 1);
         COPY_(normal_indices, puiTriangulatedIndexes, 1);
         puiTriangulatedIndexes += 1; // move past the normal
         COPY_(auIndices, puiTriangulatedIndexes, 3);
         puiTriangulatedIndexes += 3;
      }
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleFanOneNormal)
   {
      unprocessed_flags &= ~kA3DTessFaceDataTriangleFanOneNormal;
      A3DUns32 uiNbFan = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];

      for (A3DUns32 uiFan = 0; uiFan < uiNbFan; uiFan++)
      {
         A3DUns32 uiNbPoint = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++] & kA3DTessFaceDataNormalMask;

         puiTriangulatedIndexes += 1;
         A3DUns32* pFanPointIndice = puiTriangulatedIndexes;
         puiTriangulatedIndexes += 1;
         for (A3DUns32 uIPoint = 1; uIPoint < uiNbPoint - 1; uIPoint++)
         {
            COPY_(normal_indices, pFanPointIndice - 1, 1);
            COPY_(auIndices, pFanPointIndice, 1);
            COPY_(normal_indices, pFanPointIndice - 1, 1);
            COPY_(auIndices, puiTriangulatedIndexes, 1);
            COPY_(normal_indices, pFanPointIndice - 1, 1);
            COPY_(auIndices, puiTriangulatedIndexes + 1, 1);
            puiTriangulatedIndexes += 1;
         }
         puiTriangulatedIndexes += 1;
      }
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleStripeOneNormal)
   {
      unprocessed_flags &= ~kA3DTessFaceDataTriangleStripeOneNormal;
      A3DUns32 uiNbStripe = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];

      A3DUns32 uiStripe;
      for (uiStripe = 0; uiStripe < uiNbStripe; uiStripe++)
      {
         bool has_vertex_normals = 0 == (pFaceTessData->m_puiSizesTriangulated[uiCurrentSize] & kA3DTessFaceDataNormalSingle);

         A3DUns32 uiNbPoint = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++] & kA3DTessFaceDataNormalMask;
         puiTriangulatedIndexes += 2;
         A3DUns32 uIPoint, * uiNormal;
         uiNormal = puiTriangulatedIndexes - 2;
         for (uIPoint = 0; uIPoint < uiNbPoint - 2; uIPoint++)
         {
            COPY_(normal_indices, uiNormal, 1);
            COPY_(auIndices, puiTriangulatedIndexes, 1);
            if (uIPoint % 2)
            {
               COPY_(normal_indices, uiNormal, 1);
               COPY_(auIndices, puiTriangulatedIndexes - 1, 1);
               COPY_(normal_indices, uiNormal, 1);
               COPY_(auIndices, puiTriangulatedIndexes + 1, 1);
            }
            else
            {
               COPY_(normal_indices, uiNormal, 1);
               COPY_(auIndices, puiTriangulatedIndexes + 1, 1);
               COPY_(normal_indices, uiNormal, 1);
               COPY_(auIndices, puiTriangulatedIndexes - 1, 1);
            }
            puiTriangulatedIndexes += 1;
         }
         puiTriangulatedIndexes += 1;
      }
   }

   // Textured
   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleTextured)
   {
      unprocessed_flags &= ~kA3DTessFaceDataTriangleTextured;

      A3DUns32 uNbTriangles = pFaceTessData->m_puiSizesTriangulated[uiCurrentSize++];

      for (A3DUns32 uI = 0; uI < uNbTriangles; uI++)
      {
         for (int i = 0; i < 3; i++)
         {
            COPY_(normal_indices, puiTriangulatedIndexes, 1);
            puiTriangulatedIndexes += 1; // Move past the normal

            // Add to vector of texture coordinates
            COPY_(texture_indices, puiTriangulatedIndexes, pFaceTessData->m_uiTextureCoordIndexesSize);
            puiTriangulatedIndexes += pFaceTessData->m_uiTextureCoordIndexesSize;

            COPY_(auIndices, puiTriangulatedIndexes, 1);
            puiTriangulatedIndexes += 1;
         }
      }
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleFanTextured)
   {
      log(logging_function, 
          "indices_per_face_as_triangles cannot parse textured triangle data kA3DTessFaceDataTriangleFanTextured", A3D_LOG_ERROR);
      return A3D_ERROR;
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleStripeTextured)
   {
      log(logging_function, "IndicesPerFaceAsTriangles cannot parse textured triangle data kA3DTessFaceDataTriangleStripeTextured", A3D_LOG_ERROR);
      return A3D_ERROR;
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleOneNormalTextured)
   {
      log(logging_function, 
          "indices_per_face_as_triangles cannot parse textured triangle data kA3DTessFaceDataTriangleOneNormalTextured", A3D_LOG_ERROR);
      return A3D_ERROR;
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleFanOneNormalTextured)
   {
      log(logging_function, 
          "indices_per_face_as_triangles cannot parse textured triangle data kA3DTessFaceDataTriangleFanOneNormalTextured", A3D_LOG_ERROR);
      return A3D_ERROR;
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleStripeOneNormalTextured)
   {
      log(logging_function, 
          "indices_per_face_as_triangles cannot parse textured triangle data kA3DTessFaceDataTriangleStripeOneNormalTextured", A3D_LOG_ERROR);
      return A3D_ERROR;
   }

   if (unprocessed_flags != 0x0000)
   {
      log(logging_function, 
          "indices_per_face_as_triangles could not understand triangle data flag " + std::to_string(unprocessed_flags), A3D_LOG_ERROR);
      return A3D_ERROR; // there was data of a type not handled by this function.
   }

   if (uFaceIndice == sTessData.m_uiFaceTessSize - 1)
   {
      // If this is the last face, check that the triangle index data has been used up exactly
      if (puiTriangulatedIndexes != &sTessData.m_puiTriangulatedIndexes[sTessData.m_uiTriangulatedIndexSize])
      {
         log(logging_function, 
             "indices_per_face_as_triangles did not use the same number of triangle indexes as the model", A3D_LOG_ERROR);
         return A3D_ERROR;
      }
   }

   return A3D_SUCCESS;
}
//---indices_per_face_as_triangles-------------------------------------

INTERNAL PTBoolean face_in_category_cb(PTCategory cat, PTFace face)
{
   // Category selection callback to include 
   // all faces with app_surface matching the category's app_data
   void* uFaceTopoFace = (void*)PFEntityGetPointerProperty(face, PV_FACE_PROP_APP_SURFACE);
   void* uCategoryTopoFace = (void*)PFEntityGetPointerProperty(cat, PV_CATEGORY_PROP_APP_DATA);

   return (uFaceTopoFace == uCategoryTopoFace);
}
//---face_in_category_cb-----------------------------------------------

INTERNAL int traverse_shell(const A3DTopoShell* pShell,
                            A3DPolygonicaOptions* pgOpts)
{
   A3DTopoShellData sData;
   A3D_INITIALIZE_DATA(A3DTopoShellData, sData);
   PTAppSurface app_surface;

   A3DInt32 iRet = A3DTopoShellGet(pShell, &sData);
   if (iRet == A3D_SUCCESS)
   {
      // Store the surface for each CAD face in the shell
      for (A3DUns32 ui = 0; ui < sData.m_uiFaceSize; ++ui)
      {
         // Add each CAD face to m_surface_faces with the PTAppSurface index as key
         app_surface = (PTAppSurface)pgOpts->m_surface_faces.size();
         pgOpts->m_surface_faces.insert(std::make_pair(app_surface, sData.m_ppFaces[ui]));
         // Similarly store orientations with shell
         pgOpts->m_surface_face_orientations.insert(std::make_pair(app_surface, sData.m_pucOrientationWithShell[ui]));
      }

      A3DTopoShellGet(NULL, &sData);
   }
   return iRet;
}
//---traverse_shell----------------------------------------------------

INTERNAL int traverse_connex(const A3DTopoConnex* pConnex,
                             A3DPolygonicaOptions* pgOpts)
{
   A3DTopoConnexData sData;
   A3D_INITIALIZE_DATA(A3DTopoConnexData, sData);

   A3DInt32 iRet = A3DTopoConnexGet(pConnex, &sData);
   if (iRet == A3D_SUCCESS)
   {
      // Traverse each shell in the connex
      for (A3DUns32 ui = 0; ui < sData.m_uiShellSize; ++ui)
      {
         iRet = traverse_shell(sData.m_ppShells[ui], pgOpts);
      }

      A3DTopoConnexGet(NULL, &sData);
   }

   return iRet;
}
//---traverse_connex---------------------------------------------------

INTERNAL int get_brep_faces(const A3DRiBrepModel* pBrepModel, 
                            A3DPolygonicaOptions *pgOpts)
{
   // Add to the m_faces and m_orientation vectors 
   // of the A3DPolygonicaOptions structure from the A3DRiBrepModel
   A3DRiBrepModelData sBrepData;
   A3DTopoBrepDataData sBrepDataData;
   A3D_INITIALIZE_DATA(A3DRiBrepModelData, sBrepData);

   A3DInt32 iRet = A3DRiBrepModelGet(pBrepModel, &sBrepData);
   if (iRet == A3D_SUCCESS)
   {
      A3D_INITIALIZE_DATA(A3DTopoBrepDataData, sBrepDataData);

      iRet = A3DTopoBrepDataGet(sBrepData.m_pBrepData, &sBrepDataData);
      if (iRet == A3D_SUCCESS)
      {
         // Traverse each connex in the Brep
         for (A3DUns32 ui = 0; ui < sBrepDataData.m_uiConnexSize; ++ui)
         {
            iRet = traverse_connex(sBrepDataData.m_ppConnexes[ui], pgOpts);
         }

         A3DTopoBrepDataGet(NULL, &sBrepDataData);
      }

      A3DRiBrepModelGet(NULL, &sBrepData);
   }

   return iRet;
}
//---get_brep_faces----------------------------------------------------

INTERNAL void transform_texture_coord(A3DDouble& u, A3DDouble& v,
                                      A3DBool& bFlipS, A3DBool& bFlipT,
                                      A3DDouble dMatrix[16])
{
   double new_u = (u * dMatrix[0]) + (v * dMatrix[1]) + dMatrix[12];
   double new_v = (u * dMatrix[4]) + (v * dMatrix[5]) + dMatrix[13];

   if (bFlipS)
   {
      new_u = 1.0 - new_u;
   }
   if (bFlipT)
   {
      new_v = 1.0 - new_v;
   }

   u = new_u;
   v = new_v;
}
//---transform_texture_coord-------------------------------------------

INTERNAL A3DBool get_texture_transform_by_index(A3DUns32 uiTextureDefinitionIndex,
                                                A3DDouble dMatrix[16],
                                                A3DBool& bFlipS, A3DBool& bFlipT)
{
   // Return a 2D texture transform matrix given the texture definition index
   A3DStatus A3D_status = A3D_SUCCESS;
   A3DGraphTextureDefinitionData sTextureDefinitionData;

   // Set return values to defaults
   bFlipS = FALSE;
   bFlipT = FALSE;
   memset(dMatrix, 0, sizeof(A3DDouble) * 16);
   dMatrix[0] = 1.0;
   dMatrix[5] = 1.0;
   dMatrix[10] = 1.0;
   dMatrix[15] = 1.0;

   // Get texture definition data from the texture application data
   A3D_INITIALIZE_DATA(A3DGraphTextureDefinitionData, sTextureDefinitionData);
   A3D_status = A3DGlobalGetGraphTextureDefinitionData(uiTextureDefinitionIndex, &sTextureDefinitionData);
   if (A3D_status != A3D_SUCCESS)
   {
      return FALSE;
   }
   if (sTextureDefinitionData.m_pTextureTransfo == NULL)
   {
      return FALSE;
   }

   A3DGraphTextureTransformationData sTextureTransformData;
   A3D_INITIALIZE_DATA(A3DGraphTextureTransformationData, sTextureTransformData);
   A3DGraphTextureTransformationGet(sTextureDefinitionData.m_pTextureTransfo, &sTextureTransformData);

   for (int i = 0; i < 16; i++)
   {
      dMatrix[i] = sTextureTransformData.m_dMatrix[i];
   }
   bFlipS = sTextureTransformData.m_bTextureFlipS;
   bFlipT = sTextureTransformData.m_bTextureFlipT;

   A3DGraphTextureTransformationGet(NULL, &sTextureTransformData);

   return TRUE;
}
//---get_texture_transform_by_index------------------------------------

INTERNAL int get_texture_data(A3DUns32 uiTextureDefinitionIndex,
                              PTEnvironment env,
                              A3DGraphPictureData** ppPictureData)
{
   // Retrieve the texture used by the given texture definition index
   // and return a pointer to the picture data
   A3DStatus A3D_status = A3D_SUCCESS;
   PTStatus PG_status = PV_STATUS_OK;
   A3DGraphTextureDefinitionData sTextureDefinitionData;

   // Get texture definition data from the texture definition index
   A3D_INITIALIZE_DATA(A3DGraphTextureDefinitionData, sTextureDefinitionData);
   A3D_status = A3DGlobalGetGraphTextureDefinitionData(uiTextureDefinitionIndex, &sTextureDefinitionData);
   if (A3D_status != A3D_SUCCESS)
   {
      return A3D_status;
   }

   // Get picture data from the texture definition data
   *ppPictureData = new A3DGraphPictureData;
   A3D_INITIALIZE_DATA(A3DGraphPictureData, **ppPictureData);
   A3D_status = A3DGlobalGetGraphPictureData(sTextureDefinitionData.m_uiPictureIndex, *ppPictureData);
   if (A3D_status != A3D_SUCCESS)
   {
      return A3D_status;
   }

   return A3D_SUCCESS;
}
//---get_texture_data--------------------------------------------------

INTERNAL A3DBool get_texture_by_index(A3DUns32 uiTextureDefinitionIndex,
                                      A3DPolygonicaOptions& pgOpts,
                                      A3DDouble dMatrix[16],
                                      A3DBool& bFlipS, A3DBool& bFlipT,
                                      A3D_log_func logging_function)
{
   // Locate the texture in the texture palette by its index
   // or add a new entry
   // Also return the texture transform matrix

   // Search for a previously added texture with this index
   auto search = pgOpts.m_texture_palette.find(uiTextureDefinitionIndex);
   if (search == pgOpts.m_texture_palette.end())
   {
      // No texture previously created with this index - create a new one
      A3DGraphPictureData* pPictureData  = NULL;
      int PG_status = get_texture_data(uiTextureDefinitionIndex, pgOpts.m_Environment, &pPictureData);
      if (PG_status != PV_STATUS_OK)
      {
         log(logging_function, "textureByIndex - failed to create texture", A3D_LOG_ERROR);
         return FALSE;
      }
      // Add to the texture palette
      pgOpts.m_texture_palette[uiTextureDefinitionIndex] = pPictureData;
   }

   // Retrieve the 2D texture-coordinate transform for this index
   get_texture_transform_by_index(uiTextureDefinitionIndex,
                                  dMatrix, bFlipS, bFlipT);

   return TRUE;
}
//---get_texture_by_index----------------------------------------------

INTERNAL A3DBool get_face_style(A3DTessFaceData& sTessData, A3DGraphStyleData &sStyleData)
{
   // Get the colour index of the style of the given face
   A3DStatus A3D_status;

   if (sTessData.m_uiStyleIndexesSize != 1)
   {
      return FALSE;
   }

   // Default style index indicates this face does not have a specific style
   if (sTessData.m_puiStyleIndexes[0] == A3D_DEFAULT_STYLE_INDEX)
   {
      return FALSE;
   }

   A3D_INITIALIZE_DATA(A3DGraphStyleData, sStyleData);
   A3D_status = A3DGlobalGetGraphStyleData(sTessData.m_puiStyleIndexes[0], &sStyleData);
   if (A3D_status != A3D_SUCCESS)
   {
      return FALSE;
   }

   return TRUE;
}
//---get_face_style----------------------------------------------------

INTERNAL A3DBool style_is_texture(A3DGraphStyleData& sStyleData, A3DUns32 &uiTextureDefinitionIndex)
{
   // Return TRUE if the style represents a texture 
   // and set the texture definition index

   if (sStyleData.m_bMaterial)
   {
      A3DBool bMaterialIsTexture = FALSE;
      A3DGlobalIsMaterialTexture(sStyleData.m_uiRgbColorIndex, &bMaterialIsTexture);
      if (bMaterialIsTexture)
      {
         A3DGraphTextureApplicationData sTextureApplicationData;
         A3D_INITIALIZE_DATA(A3DGraphTextureApplicationData, sTextureApplicationData);
         A3DGlobalGetGraphTextureApplicationData(sStyleData.m_uiRgbColorIndex,
                                                 &sTextureApplicationData);

         uiTextureDefinitionIndex = sTextureApplicationData.m_uiTextureDefinitionIndex;

         A3DGlobalGetGraphTextureApplicationData(A3D_DEFAULT_MATERIAL_INDEX, &sTextureApplicationData);

         return TRUE;
      }
      else
      {
         return FALSE;
      }
   }
   else
   {
      return FALSE;
   }
}
//---style_is_texture--------------------------------------------------

INTERNAL double get_brep_scale(A3DRiBrepModel* pBrepModel, double default_scale)
{
   double scale = default_scale;
   
   A3DRiBrepModelData sBrepData;
   A3D_INITIALIZE_DATA(A3DRiBrepModelData, sBrepData);
   if (A3DRiBrepModelGet(pBrepModel, &sBrepData) != A3D_SUCCESS)
   {
      return scale;
   }

   A3DTopoBodyData topo_body_data;
   A3D_INITIALIZE_DATA(A3DTopoBodyData, topo_body_data);

   A3DStatus status = A3DTopoBodyGet(sBrepData.m_pBrepData, &topo_body_data);
   if (status == A3D_SUCCESS)
   {
      A3DTopoContextData context_data;
      A3D_INITIALIZE_DATA(A3DTopoContextData, context_data);

      if ((A3DTopoContextGet(topo_body_data.m_pContext, &context_data) == A3D_SUCCESS) &&
          context_data.m_bHaveScale)
      {
         scale = context_data.m_dScale;
      }

      A3DTopoContextGet(nullptr, &context_data);
   }
   A3DTopoBodyGet(nullptr, &topo_body_data);

   return scale;
}
//---get_brep_scale----------------------------------------------------

/*!
\brief Creates a PTSolid and optional mapper from the provided representation item.
\param ri The representation item to create a PTSolid from. Must be an A3DRiPolyBrep or A3DRiBrepModel
\param solid [out] solid The resultant polgonica solid
\param opts [in] Options
\return A3D_SUCCESS - Operation succeeded
  A3D_PG_NOT_INITIALIZED - Polygonica was not unlocked or initialized correctly
  A3D_PG_INVALID_RI - Representation item is unsupported type
  A3D_PG_ERROR - Internal polygonica error
*/
INTERNAL int A3DRiRepresentationItemCreatePTSolid(const A3DRiRepresentationItem* ri, 
                                                  A3DGraphStyleData &sParentStyleData, 
                                                  PTSolid* solid, 
                                                  A3DPolygonicaOptions* opts, 
                                                  A3D_log_func logging_function = nullptr)
{
   A3DEEntityType eType;
   CHECK_A3DSTATUS(A3DEntityGetType(ri, &eType), logging_function, "A3DRiRepresentationItemCreatePTSolid - failed to get type of representation item");
   if (eType != A3DEEntityType::kA3DTypeRiBrepModel && eType != A3DEEntityType::kA3DTypeRiPolyBrepModel) return A3D_PG_INVALID_RI;

   // Store A3DTopoFace and orientation data in the A3DPolygonicaOptions structure
   // for optional later query (only applied to Brep representation items)
   // Each face will be stored (for all Brep items), associated with a PTAppSurface
   if (eType == A3DEEntityType::kA3DTypeRiBrepModel)
   {
      // Add to vector of A3DRiBrepModels
      opts->m_breps.push_back((A3DRiBrepModel*)ri);
      // Associate CAD faces with app surface indices in m_surface_faces
      get_brep_faces(ri, opts);
      // Retrieve the scale for the BRep (required for subsequent points query)
      opts->m_scale = get_brep_scale((A3DRiBrepModel*)ri, opts->m_scale);
   }

   A3DStatus iRet = A3D_SUCCESS;
   PTStatus status = PV_ENTITY_NULL;

   A3DRiRepresentationItemData sRiData;
   A3D_INITIALIZE_DATA(A3DRiRepresentationItemData, sRiData);
   CHECK_A3DSTATUS(A3DRiRepresentationItemGet(ri, &sRiData), logging_function, "A3DRiRepresentationItemCreatePTSolid - A3DRiRepresentationItemGet");

   A3DTess3DData sTessData;
   A3D_INITIALIZE_DATA(A3DTess3DData, sTessData);
   A3DTess3DGet(sRiData.m_pTessBase, &sTessData);

   A3DTessBaseData sBaseTessData;
   A3D_INITIALIZE_DATA(A3DTessBaseData, sBaseTessData);
   A3DTessBaseGet(sRiData.m_pTessBase, &sBaseTessData);

   // Get vertex indices, normals and (if present) texture coordinates

   std::vector<unsigned int> auIndices;
   std::vector<PTPointer> faceAppSurface;
   std::vector<double> normals;
   std::vector<PTInt32> normal_indices;
   // Texture coordinates
   std::vector<float> texture_coords;
   PTTextureCoordinateFormat texture_coord_format = PV_TEXTURE_COORD_NULL;

   unsigned uTopoFace, uFaceSize = sTessData.m_uiFaceTessSize, uFaceAppData = 0;
   for (uTopoFace = 0; uTopoFace < uFaceSize; uTopoFace++)
   {
      std::vector<PTInt32> texture_indices;

      // Extract vertex indices, normal indices and (if present) texture indices
      // from the topo face
      indices_per_face_as_triangles(sTessData, uTopoFace, 
                                    auIndices, normal_indices, texture_indices, 
                                    logging_function);

      // Add a unique index for the topo face for use as PTAppSurface
      // one for each Polygonica face (polygon) in the topo face
      PTAppSurface app_surface = (PTPointer)(PTNat64)(opts->m_iTopoFaceCount + uTopoFace);
      faceAppSurface.insert(faceAppSurface.end(),
                            auIndices.size() / 3 - faceAppSurface.size(),
                            app_surface);

      A3DGraphStyleData sStyleData;
      A3DBool valid_face_style = get_face_style(sTessData.m_psFaceTessData[uTopoFace], sStyleData);
      A3DBool valid_texture = FALSE;
      A3DUns32 uiTextureDefinitionIndex;

      if (sTessData.m_psFaceTessData[uTopoFace].m_uiTextureCoordIndexesSize == 1)
      {
         // Topo face has texture coordinates
         // m_uiTextureCoordIndexesSize indicates sets of UV texture coordinate pairs per vertex,
         // so m_uiTextureCoordIndexesSize 1 = (u0 v0) (u1 v1) (u2 v2)
         // m_uiTextureCoordIndexesSize 2 = (u00 v00 u01 v01) (u10 v10 u11 v11) (u20 v20 u21 v21)
         // Here we only handle m_uiTextureCoordIndexesSize == 1

         // Set Polygonica texture coordinate format to indicate a single pair (UV) of floats
         texture_coord_format = PV_TEXTURE_COORD_FLOAT_ARRAY_2;

         A3DDouble textureTransformMatrix[16];
         A3DBool bFlipS = FALSE;
         A3DBool bFlipT = FALSE;

         // Find a texture from the face style or its parent style
         // This will add the texture to the texture palette
         if (valid_face_style &&
             style_is_texture(sStyleData, uiTextureDefinitionIndex))
         {
            valid_texture = get_texture_by_index(uiTextureDefinitionIndex, *opts,
                                                 textureTransformMatrix, bFlipS, bFlipT,
                                                 logging_function);
         }
         else if (style_is_texture(sParentStyleData, uiTextureDefinitionIndex))
         {
            valid_texture = get_texture_by_index(uiTextureDefinitionIndex, *opts,
                                                 textureTransformMatrix, bFlipS, bFlipT,
                                                 logging_function);
         }

         // If there is a texture for this face, 
         // add the index for this app surface to the output data structure
         if (valid_texture)
         {
            opts->m_surface_textures.insert(std::make_pair(app_surface, uiTextureDefinitionIndex));
         }

         // Generate a vector of texture coordinates from the texture indices
         for (int i = 0; i < texture_indices.size(); i++)
         {
            double u = sTessData.m_pdTextureCoords[texture_indices[i]];
            double v = sTessData.m_pdTextureCoords[texture_indices[i] + 1];
            if (valid_texture)
            {
               transform_texture_coord(u, v, bFlipS, bFlipT, textureTransformMatrix);
            }
            // Invert v for Polygonica textures
            v = 1.0 - v;

            texture_coords.push_back((float)u);
            texture_coords.push_back((float)v);
         }
         texture_indices.clear();
      }
      else
      {
         // Check for a valid colour
         A3DDouble r(0.76f), g(0.77f), b(0.79f);
         if (valid_face_style &&
             !style_is_texture(sStyleData, uiTextureDefinitionIndex))
         {
            get_color_from_graphic_data(sStyleData, r, g, b, logging_function);

            double* rgbcolour = new double[3];
            rgbcolour[0] = r;
            rgbcolour[1] = g;
            rgbcolour[2] = b;
            opts->m_surface_colours.insert(std::make_pair(app_surface, rgbcolour));
         }
      }
   }

   // Increment the CAD face count in the output structure
   // by the number of CAD faces in this solid
   opts->m_iTopoFaceCount += uFaceSize;

   for (int i = 0; i < auIndices.size(); i++)
   {
      auIndices[i] = auIndices[i] / 3;
   }

   for (int i = 0; i < normal_indices.size(); i++)
   {
      normal_indices[i] = normal_indices[i] / 3;
   }

   // Set mesh options to supply vertex normals and (if present) texture coordinates
   PTMeshSolidOpts meshOpts;
   PMInitMeshSolidOpts(&meshOpts);
   meshOpts.normals = (PTVector*)sTessData.m_pdNormals;
   meshOpts.normal_indices = normal_indices.data();
   meshOpts.app_surfaces = (PTPointer*)faceAppSurface.data();
   if (texture_coord_format != PV_TEXTURE_COORD_NULL)
   {
      meshOpts.texture_coord_format = texture_coord_format;
      meshOpts.texture_coordinates = texture_coords.data();
   }

   // Create the solid given vertex indices, normals and (if present) texture coordinates
   status = PFSolidCreateFromMesh(opts->m_Environment,
                                  (PTNat32)(auIndices.size() / 3),   // Total number of triangles
                                  NULL,                              // No internal loops
                                  NULL,                              // All faces are triangles
                                  auIndices.data(),                  // Indices into vertex array
                                  sBaseTessData.m_pdCoords,          // Pointer to vertex array
                                  &meshOpts,
                                  solid);                            // Resultant PG solid

   // TODO: Should a failure here set iRet to a failure status?
   CHECK_PTSTATUS(status, logging_function, "A3DRiRepresentationItemCreatePTSolid - PFSolidCreateFromMesh");

   // Create a group of polygons for each surface in the solid
   if (status == PV_STATUS_OK)
   {
      PTEntityList solid_polygons = PV_ENTITY_NULL;
      PFEntityCreateEntityList(*solid, PV_ENTITY_TYPE_FACE, NULL, &solid_polygons);

      // Add each polygon in the solid to a group associated with its PTAppSurface
      for (PTEntity polygon = PFEntityListGetFirst(solid_polygons);
           polygon != PV_ENTITY_NULL;
           polygon = PFEntityListGetNext(solid_polygons, polygon))
      {
         // Retrieve the app surface for this polygon
         PTAppSurface app_surface = (PTAppSurface)PFEntityGetPointerProperty(polygon, PV_FACE_PROP_APP_SURFACE);

         // Retrieve the group for the app surface (or create one if necessary)
         PTEntityGroup surface_polygons = PV_ENTITY_NULL;

         auto search = opts->m_surface_groups.find(app_surface);
         if (search == opts->m_surface_groups.end())
         {
            PFEntityGroupCreate(opts->m_Environment, &surface_polygons);
            opts->m_surface_groups.insert(std::make_pair(app_surface, surface_polygons));
         }
         else
         {
            surface_polygons = (PTEntityGroup)search->second;
         }
         // Add the polygon to the group
         PFEntityGroupAddEntity(surface_polygons, polygon);
      }
      PFEntityListDestroy(solid_polygons, 0);
   }

   A3DRiRepresentationItemGet(NULL, &sRiData);
   A3DTess3DGet(NULL, &sTessData);
   A3DTessBaseGet(NULL, &sBaseTessData);

   return iRet;
}
//---A3DRiRepresentationItemCreatePTSolid------------------------------

INTERNAL int traverse_rep_item(const A3DRiRepresentationItem* pRepItem,
                               std::vector<void*> assemblyPath,
                               PTTransformMatrix transform,
                               A3DMiscCascadedAttributes* pFatherAttr,
                               A3DPolygonicaOptions& pgOpts,
                               A3D_log_func logging_function);

INTERNAL int traverse_set(const A3DRiSet* pSet,
                          std::vector<void*> assemblyPath,
                          PTTransformMatrix transform,
                          A3DMiscCascadedAttributes* pAttr,
                          A3DPolygonicaOptions& pgOpts,
                          A3D_log_func logging_function)
{
   A3DInt32 iRet = A3D_SUCCESS;
   A3DRiSetData sData;
   A3D_INITIALIZE_DATA(A3DRiSetData, sData);

   iRet = A3DRiSetGet(pSet, &sData);
   if (iRet == A3D_SUCCESS)
   {
      for (A3DUns32 ui = 0; ui < sData.m_uiRepItemsSize; ++ui)
      {
         iRet = traverse_rep_item(sData.m_ppRepItems[ui], assemblyPath, transform, pAttr, pgOpts, logging_function);
      }

      A3DRiSetGet(NULL, &sData);
   }

   return iRet;
}
//---traverse_set------------------------------------------------------

INTERNAL A3DStatus multiply_matrix(const double* padFather,
                                   const double* pdThisMatrix, 
                                   double* pdResult)
{
   A3DStatus iRet = A3D_SUCCESS;
   pdResult[0] = padFather[0] * pdThisMatrix[0] + padFather[4] * pdThisMatrix[1] + padFather[8] * pdThisMatrix[2] + padFather[12] * pdThisMatrix[3];
   pdResult[1] = padFather[1] * pdThisMatrix[0] + padFather[5] * pdThisMatrix[1] + padFather[9] * pdThisMatrix[2] + padFather[13] * pdThisMatrix[3];
   pdResult[2] = padFather[2] * pdThisMatrix[0] + padFather[6] * pdThisMatrix[1] + padFather[10] * pdThisMatrix[2] + padFather[14] * pdThisMatrix[3];
   pdResult[3] = padFather[3] * pdThisMatrix[0] + padFather[7] * pdThisMatrix[1] + padFather[11] * pdThisMatrix[2] + padFather[15] * pdThisMatrix[3];
   pdResult[4] = padFather[0] * pdThisMatrix[4] + padFather[4] * pdThisMatrix[5] + padFather[8] * pdThisMatrix[6] + padFather[12] * pdThisMatrix[7];
   pdResult[5] = padFather[1] * pdThisMatrix[4] + padFather[5] * pdThisMatrix[5] + padFather[9] * pdThisMatrix[6] + padFather[13] * pdThisMatrix[7];
   pdResult[6] = padFather[2] * pdThisMatrix[4] + padFather[6] * pdThisMatrix[5] + padFather[10] * pdThisMatrix[6] + padFather[14] * pdThisMatrix[7];
   pdResult[7] = padFather[3] * pdThisMatrix[4] + padFather[7] * pdThisMatrix[5] + padFather[11] * pdThisMatrix[6] + padFather[15] * pdThisMatrix[7];
   pdResult[8] = padFather[0] * pdThisMatrix[8] + padFather[4] * pdThisMatrix[9] + padFather[8] * pdThisMatrix[10] + padFather[12] * pdThisMatrix[11];
   pdResult[9] = padFather[1] * pdThisMatrix[8] + padFather[5] * pdThisMatrix[9] + padFather[9] * pdThisMatrix[10] + padFather[13] * pdThisMatrix[11];
   pdResult[10] = padFather[2] * pdThisMatrix[8] + padFather[6] * pdThisMatrix[9] + padFather[10] * pdThisMatrix[10] + padFather[14] * pdThisMatrix[11];
   pdResult[11] = padFather[3] * pdThisMatrix[8] + padFather[7] * pdThisMatrix[9] + padFather[11] * pdThisMatrix[10] + padFather[15] * pdThisMatrix[11];
   pdResult[12] = padFather[0] * pdThisMatrix[12] + padFather[4] * pdThisMatrix[13] + padFather[8] * pdThisMatrix[14] + padFather[12] * pdThisMatrix[15];
   pdResult[13] = padFather[1] * pdThisMatrix[12] + padFather[5] * pdThisMatrix[13] + padFather[9] * pdThisMatrix[14] + padFather[13] * pdThisMatrix[15];
   pdResult[14] = padFather[2] * pdThisMatrix[12] + padFather[6] * pdThisMatrix[13] + padFather[10] * pdThisMatrix[14] + padFather[14] * pdThisMatrix[15];
   pdResult[15] = padFather[3] * pdThisMatrix[12] + padFather[7] * pdThisMatrix[13] + padFather[11] * pdThisMatrix[14] + padFather[15] * pdThisMatrix[15];
   return iRet;
}
//---multiply_matrix---------------------------------------------------

INTERNAL A3DVector3dData cross_product(const A3DVector3dData* X, const A3DVector3dData* Y)
{
   A3DVector3dData Z;
   Z.m_dX = X->m_dY * Y->m_dZ - X->m_dZ * Y->m_dY;
   Z.m_dY = X->m_dZ * Y->m_dX - X->m_dX * Y->m_dZ;
   Z.m_dZ = X->m_dX * Y->m_dY - X->m_dY * Y->m_dX;
   return Z;
}
//---cross_product-----------------------------------------------------

INTERNAL A3DStatus cartesian_transform(A3DMiscTransformation* pTransformation,
                                       PTTransformMatrix transform, 
                                       PTTransformMatrix& localTransform, 
                                       A3D_log_func logging_function)
{
   A3DStatus iRet = A3D_SUCCESS;

   A3DMiscCartesianTransformationData sTransformData;
   A3D_INITIALIZE_DATA(A3DMiscCartesianTransformationData, sTransformData);
   iRet = A3DMiscCartesianTransformationGet(pTransformation, &sTransformData);

   if (iRet == A3D_SUCCESS)
   {
      double dMirror = (sTransformData.m_ucBehaviour & kA3DTransformationMirror) ? -1. : 1.;
      A3DVector3dData sZVector;
      double m[16];
      memset(m, 0, 16 * sizeof(double));
      sZVector = cross_product(&(sTransformData.m_sXVector), &(sTransformData.m_sYVector));

      m[12] = sTransformData.m_sOrigin.m_dX;
      m[13] = sTransformData.m_sOrigin.m_dY;
      m[14] = sTransformData.m_sOrigin.m_dZ;

      m[0] = sTransformData.m_sXVector.m_dX * sTransformData.m_sScale.m_dX;
      m[1] = sTransformData.m_sXVector.m_dY * sTransformData.m_sScale.m_dX;
      m[2] = sTransformData.m_sXVector.m_dZ * sTransformData.m_sScale.m_dX;

      m[4] = sTransformData.m_sYVector.m_dX * sTransformData.m_sScale.m_dY;
      m[5] = sTransformData.m_sYVector.m_dY * sTransformData.m_sScale.m_dY;
      m[6] = sTransformData.m_sYVector.m_dZ * sTransformData.m_sScale.m_dY;

      m[8] = dMirror * sZVector.m_dX * sTransformData.m_sScale.m_dZ;
      m[9] = dMirror * sZVector.m_dY * sTransformData.m_sScale.m_dZ;
      m[10] = dMirror * sZVector.m_dZ * sTransformData.m_sScale.m_dZ;

      m[15] = 1.;

      multiply_matrix((double*)transform, m, (double*)localTransform);

      A3DMiscCartesianTransformationGet(NULL, &sTransformData);
   }

   return iRet;
}
//---cartesian_transform-----------------------------------------------

INTERNAL A3DStatus general_transform(A3DMiscTransformation* pTransformation,
                                     PTTransformMatrix transform,
                                     PTTransformMatrix& localTransform,
                                     A3D_log_func logging_function)
{
   A3DStatus iRet = A3D_SUCCESS;

   A3DMiscGeneralTransformationData sTransformData;
   A3D_INITIALIZE_DATA(A3DMiscGeneralTransformationData, sTransformData);
   iRet = A3DMiscGeneralTransformationGet(pTransformation, &sTransformData);

   if (iRet == A3D_SUCCESS)
   {
      multiply_matrix((double*)transform, sTransformData.m_adCoeff, (double*)localTransform);

      A3DMiscGeneralTransformationGet(NULL, &sTransformData);
   }

   return iRet;
}
//---general_transform-------------------------------------------------

INTERNAL A3DStatus get_transform(A3DMiscTransformation* pTransformation,
                                 PTTransformMatrix transform,
                                 PTTransformMatrix& localTransform,
                                 A3D_log_func logging_function)
{
   A3DStatus iRet = A3D_SUCCESS;
   A3DEEntityType eType = kA3DTypeUnknown;
   iRet = A3DEntityGetType(pTransformation, &eType);
   if (eType == kA3DTypeMiscCartesianTransformation)
   {
      iRet = cartesian_transform(pTransformation, transform, localTransform, logging_function);
   }
   else if (eType == kA3DTypeMiscGeneralTransformation)
   {
      iRet = general_transform(pTransformation, transform, localTransform, logging_function);
   }
   else
   {
      log(logging_function, "In get_transform, the location type " + std::to_string(eType) + " is unknown", A3D_LOG_ERROR);
   }

   return iRet;
}
//---get_transform-----------------------------------------------------

INTERNAL int traverse_rep_item(const A3DRiRepresentationItem* pRepItem,
                               std::vector<void*> assemblyPath,
                               PTTransformMatrix transform,
                               A3DMiscCascadedAttributes* pFatherAttr,
                               A3DPolygonicaOptions& pgOpts,
                               A3D_log_func logging_function)
{
   A3DInt32 iRet = A3D_SUCCESS;
   PTStatus status = PV_STATUS_OK;
   A3DEEntityType eType;

   A3DMiscCascadedAttributes* pAttr;
   A3DMiscCascadedAttributesData sAttrData;
   CHECK_A3DSTATUS(create_and_push_cascaded_attributes(pRepItem, pFatherAttr, &pAttr, &sAttrData, logging_function),
                   logging_function, "traverse_rep_item - stCreateAndPushCascadedAttributes");
   const MiscCascadedAttributesGuard sMCAttrGuard(pAttr);

   CHECK_A3DSTATUS(A3DEntityGetType(pRepItem, &eType),
                   logging_function, "traverse_rep_item - A3DEntityGetType");

   switch (eType)
   {
      case kA3DTypeRiSet:
      {
         iRet = traverse_set(pRepItem, assemblyPath, transform, pAttr, pgOpts, logging_function);
         break;
      }
      case kA3DTypeRiBrepModel:
      case kA3DTypeRiPolyBrepModel:
      {
         A3DRiRepresentationItemData sData;
         A3D_INITIALIZE_DATA(A3DRiRepresentationItemData, sData);
         iRet = A3DRiRepresentationItemGet(pRepItem, &sData);

         PTTransformMatrix localTransform;
         memcpy(localTransform, transform, 16 * sizeof(double));

         if (sData.m_pCoordinateSystem)
         {
            A3DRiCoordinateSystemData sCoordSysData;
            A3D_INITIALIZE_DATA(A3DRiCoordinateSystemData, sCoordSysData);
            iRet = A3DRiCoordinateSystemGet(sData.m_pCoordinateSystem, &sCoordSysData);

            iRet = get_transform(sCoordSysData.m_pTransformation, transform, localTransform, logging_function);

            A3DRiCoordinateSystemGet(NULL, &sCoordSysData);
         }

         A3DRiRepresentationItemGet(NULL, &sData);

         // Create a PTSolid and add a representation item / solid pair to the output map m_parts
         PTSolid solid;
         if (pgOpts.m_parts.find(pRepItem) == pgOpts.m_parts.end())
         {
            iRet = A3DRiRepresentationItemCreatePTSolid(pRepItem, sAttrData.m_sStyle, 
                                                        &solid, &pgOpts);
            pgOpts.m_parts.insert(std::make_pair(pRepItem, solid));
         }
         else
         {
            solid = pgOpts.m_parts[pRepItem];
         }

         // Create a world entity for this representation item instance
         PTWorldEntity worldEntity;
         status = PFWorldAddEntity(pgOpts.m_World, solid, &worldEntity);
         CHECK_PTSTATUS(status, logging_function, "traverse_rep_item - PFWorldAddEntity");
         if (status == PV_STATUS_OK)
         {
            // Set the position transform
            status = PFWorldEntitySetTransform(worldEntity, localTransform, NULL);

            // Retrieve the colour for the part (unless using a texture)
            A3DUns32 texture_index = 0;
            // Use standard model grey as a default colour
            A3DDouble r(0.76f), g(0.77f), b(0.79f);
            if (!style_is_texture(sAttrData.m_sStyle, texture_index))
            {
               CHECK_A3DSTATUS(get_color_from_graphic_data(sAttrData.m_sStyle, r, g, b, logging_function),
                              logging_function, "traverse_rep_item - stExtractColorFromGraphicData");
            }

            // Add a world entity / colour pair to the output colour map m_entity_colours
            A3DDouble* rgbcolour = new A3DDouble[3];
            rgbcolour[0] = r; 
            rgbcolour[1] = g; 
            rgbcolour[2] = b;
            pgOpts.m_entity_colours.insert(std::make_pair(worldEntity, rgbcolour));

            // If using Polygonica rendering,
            // set render style of the world entity from the colour
#ifdef PGRENDER_HDR
            set_render_style_from_colour(pgOpts.m_Environment,
                                         worldEntity, rgbcolour);
#endif

            // Add a world entity / path pair to the output map m_paths
            std::vector<void*>* path = new std::vector<void*>(assemblyPath);
            pgOpts.m_paths.insert(std::make_pair(worldEntity, path));

            // Add the world entity to the output vector m_entities
            pgOpts.m_entities.push_back(worldEntity);
            PFEntityGetEntityProperty(worldEntity, PV_WENTITY_PROP_ENTITY);
         }
         break;
      }
      default:
      {
         iRet = A3D_NOT_IMPLEMENTED;
         log(logging_function, "traverse_rep_item of type " + std::to_string(eType) + " is not implemented", A3D_LOG_WARN);
         break;
      }
   }
   return iRet;
}
//---traverse_rep_item-------------------------------------------------

INTERNAL int traverse_part_def(const A3DAsmPartDefinition* pPart,
                               std::vector<void*> assemblyPath, 
                               PTTransformMatrix transform, 
                               A3DMiscCascadedAttributes* pFatherAttr, 
                               A3DPolygonicaOptions& pgOpts, 
                               A3D_log_func logging_function)
{
   A3DInt32 iRet = A3D_SUCCESS;

   A3DMiscCascadedAttributes* pAttr;
   A3DMiscCascadedAttributesData sAttrData;
   CHECK_A3DSTATUS(create_and_push_cascaded_attributes(pPart, pFatherAttr, &pAttr, &sAttrData, logging_function),
                   logging_function, "traverse_part_def - create_and_push_cascaded_attributes");
   const MiscCascadedAttributesGuard sMCAttrGuard(pAttr);

   A3DAsmPartDefinitionData sData;
   A3D_INITIALIZE_DATA(A3DAsmPartDefinitionData, sData);

   assemblyPath.push_back((void*)pPart);

   iRet = A3DAsmPartDefinitionGet(pPart, &sData);
   if (iRet == A3D_SUCCESS)
   {
      A3DUns32 ui;

      for (ui = 0; ui < sData.m_uiRepItemsSize; ++ui)
      {
         traverse_rep_item(sData.m_ppRepItems[ui], assemblyPath, transform, pAttr, pgOpts, logging_function);
      }

      A3DAsmPartDefinitionGet(NULL, &sData);
   }

   assemblyPath.pop_back();

   return iRet;
}
//---traverse_part_def-------------------------------------------------

INTERNAL int traverse_product_occurrence(const A3DAsmProductOccurrence* pOccurrence,
                                         std::vector<void*> assemblyPath, 
                                         PTTransformMatrix transform, 
                                         A3DMiscCascadedAttributes* pFatherAttr, 
                                         bool isPrototype, 
                                         A3DPolygonicaOptions& pgOpts, 
                                         A3D_log_func logging_function)
{
   A3DInt32 iRet = A3D_SUCCESS;

   A3DMiscCascadedAttributes* pAttr;
   A3DMiscCascadedAttributesData sAttrData;
   CHECK_A3DSTATUS(create_and_push_cascaded_attributes(pOccurrence, pFatherAttr, &pAttr, &sAttrData, logging_function),
                   logging_function, "traverse_product_occurrence - create_and_push_cascaded_attributes");
   const MiscCascadedAttributesGuard sMCAttrGuard(pAttr);

   PTTransformMatrix localTransform;
   memcpy(localTransform, transform, 16 * sizeof(double));

   A3DAsmProductOccurrenceData sData;
   A3D_INITIALIZE_DATA(A3DAsmProductOccurrenceData, sData);
   iRet = A3DAsmProductOccurrenceGet(pOccurrence, &sData);

   if (iRet == A3D_SUCCESS)
   {
      A3DUns32 ui;

      if (sData.m_pLocation)
      {
         A3DEEntityType eType = kA3DTypeUnknown;
         iRet = A3DEntityGetType(sData.m_pLocation, &eType);
         if ((eType == kA3DTypeMiscCartesianTransformation) ||
             (eType == kA3DTypeMiscGeneralTransformation))
         {
            iRet = get_transform(sData.m_pLocation, transform, localTransform, logging_function);
         }
         else
         {
            log(logging_function, "In traverse_product_occurrence, the location type " + std::to_string(eType) + " is unknown", A3D_LOG_ERROR);
         }
      }

      if (!isPrototype)
      {
         assemblyPath.push_back((void*)pOccurrence);
      }

      if (sData.m_pPrototype)
      {
         traverse_product_occurrence(sData.m_pPrototype, assemblyPath, localTransform, pAttr, true, pgOpts, logging_function);
      }
      else if (sData.m_pExternalData)
      {
         traverse_product_occurrence(sData.m_pExternalData, assemblyPath, localTransform, pAttr, true, pgOpts, logging_function);
      }
      else
      {
         for (ui = 0; ui < sData.m_uiPOccurrencesSize; ++ui)
         {
            traverse_product_occurrence(sData.m_ppPOccurrences[ui], assemblyPath, localTransform, pAttr, false, pgOpts, logging_function);
         }
      }

      if (sData.m_pPart)
      {
         traverse_part_def(sData.m_pPart, assemblyPath, localTransform, pAttr, pgOpts, logging_function);
      }

      if (!isPrototype)
      {
         assemblyPath.pop_back();
      }
      CHECK_A3DSTATUS(A3DAsmProductOccurrenceGet(NULL, &sData), logging_function, "traverse_product_occurrence - A3DAsmProductOccurrenceGet");
   }

   return iRet;
}
//---traverse_product_occurrence---------------------------------------

/*!
\brief Creates a Polygonica world and PTSolids list from the provided model.
\param pModelFile The model file to parse solids and transforms. Should contain A3DRiPolyBrep or A3DRiBrepModel
\param A3DPolygonicaOptions [out] opts The structure containing the resultant world populated with solids
\return A3D_SUCCESS - Operation succeeded
  A3D_PG_NOT_INITIALIZED - Polygonica was not unlocked or initialized correctly
  A3D_PG_INVALID_RI - Representation item is unsupported type
  A3D_PG_ERROR - Internal Polygonica error
*/
INTERNAL int A3DModelCreatePGWorld(const A3DAsmModelFile* pModelFile,
                                   A3DPolygonicaOptions& pgOpts, 
                                   A3D_log_func logging_function = nullptr)
{
   A3DStatus iRet = A3D_SUCCESS;

   A3DAsmModelFileData sData;
   A3D_INITIALIZE_DATA(A3DAsmModelFileData, sData);

   PTTransformMatrix transform;
   PMInitTransformMatrix(transform);

   std::vector<void*> assemblyPath;

   // Allocate cascaded attributes
   A3DMiscCascadedAttributes* pAttr;
   CHECK_A3DSTATUS(A3DMiscCascadedAttributesCreate(&pAttr), logging_function, "A3DModelCreatePGWorld");
   const MiscCascadedAttributesGuard sMCAttrGuard(pAttr);

   iRet = A3DAsmModelFileGet(pModelFile, &sData);
   if (iRet == A3D_SUCCESS)
   {
      // Reset the A3DPolygonicaOptions structure (in case it is being re-used)
      A3DDestroyBridgeData(pgOpts);

      // Traverse all product occurrences, 
      // creating PTSolids and PTWorldEntities 
      // and populating the A3DPolygonicaOptions structure
      for (A3DUns32 ui = 0; ui < sData.m_uiPOccurrencesSize; ++ui)
      {
         traverse_product_occurrence(sData.m_ppPOccurrences[ui], assemblyPath, transform, pAttr, false, pgOpts, logging_function);
      }
      CHECK_A3DSTATUS(A3DAsmModelFileGet(NULL, &sData), logging_function, "A3DModelCreatePGWorld - A3DAsmModelFileGet");
   }

   // For surfaces with specified colours, 
   // set colours on PTFaces belonging to the surface
   set_pg_surface_colours(pgOpts);

   return iRet;
}
//---A3DModelCreatePGWorld---------------------------------------------

INTERNAL PTStatus pg_mesh_begin_callback(PTPointer callback_app_data,
                                         PTNat32 num_faces,
                                         PTNat32 num_loops,
                                         PTNat32 num_vertex_indices,
                                         PTNat32 num_vertices,
                                         PTNat32 vertex_array_size,
                                         PTPoint** vertices,
                                         PTPointer** vertex_app_data)
{
   PGMeshCallbackData* pdata = (PGMeshCallbackData*)callback_app_data;
   PTNat32 i;

   pdata->num_points = num_vertices;
   pdata->points = (PTPoint*)malloc(sizeof(PTPoint) * num_vertices);
   pdata->num_triangles = num_faces;
   pdata->indices = (PTNat32*)malloc(sizeof(PTNat32) * num_vertex_indices);
   pdata->normals = (PTPoint*)malloc(sizeof(PTPoint) * num_vertex_indices);

   for (i = 0; i < num_vertices; i++)
   {
      PFMeshGetVertexPosition(pdata->points[i], vertices, vertex_array_size, i);
   }
   return PV_STATUS_OK;
}
//---pg_mesh_begin_callback--------------------------------------------

INTERNAL PTStatus pg_add_polygon_callback(struct PTMeshPolygon* polygon)
{
   int i;
   PGMeshCallbackData* pdata = (PGMeshCallbackData*)polygon->callback_app_data;

   for (i = 0; i < 3; i++)
   {
      // Add indices of vertices of this triangle to the indices list associated with this surface
      pdata->surfaces[polygon->app_surface].indices.push_back(polygon->indices[i]);

      // Add normals of vertices of this triangle to the normals list associated with this surface
      // 3 doubles per normal
      pdata->surfaces[polygon->app_surface].normals.push_back(polygon->normals[i * 3]);
      pdata->surfaces[polygon->app_surface].normals.push_back(polygon->normals[(i * 3) + 1]);
      pdata->surfaces[polygon->app_surface].normals.push_back(polygon->normals[(i * 3) + 2]);
   }
   // If this is the first polygon added to this surface, initialise has_colour
   if (pdata->surfaces[polygon->app_surface].indices.size() == 3)
   {
      pdata->surfaces[polygon->app_surface].has_colour = false;
   }
   // If a colour has not been set for the surface, check for one from this polygon
   // NB polygon->face_colour will be NULL if no PTFace colour is set
   if ((polygon->face_colour != NULL) &&
       !pdata->surfaces[polygon->app_surface].has_colour)
   {
      double* colour = (double*)polygon->face_colour;
      pdata->surfaces[polygon->app_surface].colour[0] = colour[0];
      pdata->surfaces[polygon->app_surface].colour[1] = colour[1];
      pdata->surfaces[polygon->app_surface].colour[2] = colour[2];
      pdata->surfaces[polygon->app_surface].has_colour = true;
   }

   return PV_STATUS_OK;
}
//---pg_add_polygon_callback-------------------------------------------

INTERNAL PTStatus pg_mesh_end_callback(PTPointer callback_app_data)
{
   // Copy the indices and normals from get_mesh_callback_data.surfaces 
   // to indices and normals arrays 
   // so indices and normals are sorted by app surface
   PTNat32 i;
   int index = 0;
   int surface_index;
   PTNat32 num_indices;
   PGMeshCallbackData* pdata = (PGMeshCallbackData*)callback_app_data;

   pdata->num_surfaces = (PTNat32)pdata->surfaces.size();
   pdata->polygons_per_surface = (PTNat32*)malloc(sizeof(PTNat32) * pdata->num_surfaces);
   pdata->surface_colours = (double**)malloc(sizeof(double*) * pdata->num_surfaces);

   surface_index = 0;
   for (auto surface = pdata->surfaces.begin(); 
        surface != pdata->surfaces.end(); 
        surface++, surface_index++)
   {
      num_indices = (PTNat32)surface->second.indices.size();
      pdata->polygons_per_surface[surface_index] = num_indices / 3;
      for (i = 0; i < num_indices; i++, index++)
      {
         pdata->indices[index] = surface->second.indices[i];
         pdata->normals[index][0] = surface->second.normals[i * 3];
         pdata->normals[index][1] = surface->second.normals[(i * 3) + 1];
         pdata->normals[index][2] = surface->second.normals[(i * 3) + 2];
      }

      if (surface->second.has_colour)
      {
         pdata->surface_colours[surface_index] = (double*)malloc(sizeof(double) * 3);
         pdata->surface_colours[surface_index][0] = surface->second.colour[0];
         pdata->surface_colours[surface_index][1] = surface->second.colour[1];
         pdata->surface_colours[surface_index][2] = surface->second.colour[2];
      }
      else
      {
         pdata->surface_colours[surface_index] = NULL;
      }
   }

   return PV_STATUS_OK;
}
//---pg_mesh_end_callback----------------------------------------------

INTERNAL PTStatus get_solid_mesh_data(PTSolid   solid,
                                      PTNat32*  num_points,
                                      PTPoint** points,
                                      PTNat32*  num_triangles,
                                      PTNat32** indices,
                                      PTPoint** normals, 
                                      PTNat32*  num_surfaces,
                                      PTNat32** polygons_per_surface,
                                      double*** surface_colours)
{
   // Retrieve the triangle mesh data of the solid as 
   // an array of points (size num_points), 
   // an array of indices into the points array (size 3 * num_triangles) 
   // an array of vertex normals (size 3 * num_triangles) 
   // an array of number of polygons in each surface/CAD face (size num_surfaces)
   // an array of colours (double[3] or NULL) for each surface/CAD face (size num_surfaces)
   // NB indices and normals are ordered by surface 
   // e.g. all polygons_per_surface[0] * 3 indices of the first surface occur first in indices, 
   // then all polygons_per_surface[1] * 3 indices of the second surface etc.
   PTStatus status = PV_STATUS_OK;
   PGMeshCallbackData data;
   PTGetMeshOpts mesh_options;

   PMInitGetMeshOpts(&mesh_options);
   mesh_options.app_data = &data;
   mesh_options.begin_callback = pg_mesh_begin_callback;
   mesh_options.add_polygon_callback = pg_add_polygon_callback;
   mesh_options.end_callback = pg_mesh_end_callback;
   mesh_options.output_vertex_normals = TRUE;
   mesh_options.output_app_surfaces = TRUE;
   mesh_options.output_face_colours = TRUE;
   mesh_options.colour_format = PV_COLOUR_DOUBLE_RGB_ARRAY;
   status = PFSolidGetMesh(solid, PV_MESH_TRIANGLES, &mesh_options);

   *num_points = data.num_points;
   *points = data.points;
   *num_triangles = data.num_triangles;
   *indices = data.indices;
   *normals = data.normals;
   *num_surfaces = data.num_surfaces;
   *polygons_per_surface = data.polygons_per_surface;
   *surface_colours = data.surface_colours;

   return status;
}
//---get_solid_mesh_data-----------------------------------------------

INTERNAL A3DBool create_color_style(A3DDouble dRed, A3DDouble dGreen, A3DDouble dBlue,
                                    A3DUns32& uiStyleIndex)
{
   // Create a color style
   A3DUns32 uiColorIndex = 0;

   A3DStatus iRet = A3D_SUCCESS;
   A3DGraphRgbColorData sData;
   A3D_INITIALIZE_DATA(A3DGraphRgbColorData, sData);
   sData.m_dRed = dRed;
   sData.m_dGreen = dGreen;
   sData.m_dBlue = dBlue;
   iRet = A3DGlobalInsertGraphRgbColor(&sData, &uiColorIndex);
   if (iRet != A3D_SUCCESS)
   {
      return false;
   }

   A3DGraphStyleData sStyleData;
   A3D_INITIALIZE_DATA(A3DGraphStyleData, sStyleData);
   /*
      sStyleData.m_bMaterial = false;
      sStyleData.m_bVPicture = false;
      sStyleData.m_dWidth = 0.1; // default
      sStyleData.m_bIsTransparencyDefined = false;
      sStyleData.m_ucTransparency = 0;
      sStyleData.m_bSpecialCulling = false;
      sStyleData.m_bBackCulling = false;
   */
   sStyleData.m_uiRgbColorIndex = uiColorIndex;
   iRet = A3DGlobalInsertGraphStyle(&sStyleData, &uiStyleIndex);

   return iRet == A3D_SUCCESS;
}
//---create_color_style------------------------------------------------

INTERNAL int create_tessellation(A3DTess3D** ppTess3D,
                                 PTSolid solid, 
                                 A3D_log_func logging_function = nullptr)
{
   // Create a HOOPS tessellation object from the solid
   PTStatus status = PV_STATUS_OK;
   PTNat32  num_points = 0;
   PTPoint* points = NULL;
   PTNat32  num_triangles = 0;
   PTNat32* indices = NULL;
   PTPoint* normals = NULL;
   PTNat32  num_surfaces = 0;
   PTNat32* polygons_per_surface = NULL;
   double** surface_colours = NULL;

   PTNat32  p, t, v, s;
   int      vertex_index;
   int      triangulated_index;
   int      normal_index;
   A3DUns32 first_surface_index = 0;

   A3DTessBaseData  sTessBaseData;
   A3DTess3DData    sTess3DData;

   //---------------------------------------------------------------------
   // 1) Create arrays of points, vertex indices, vertex normals, 
   // and polygons per surface from the solid mesh
   //---------------------------------------------------------------------
   status = get_solid_mesh_data(solid,
                                &num_points, &points,
                                &num_triangles, &indices, &normals,
                                &num_surfaces, &polygons_per_surface, &surface_colours);
   CHECK_PTSTATUS(status, logging_function,
                  "create_tessellation - getSolidMeshData");
   if (status != PV_STATUS_OK)
   {
      return A3D_PG_ERROR;
   }

   //---------------------------------------------------------------------
   // 2) Create A3DTessBaseData - point data
   //---------------------------------------------------------------------
   A3D_INITIALIZE_DATA(A3DTessBaseData, sTessBaseData);

   // 3 doubles for each point
   sTessBaseData.m_uiCoordSize = num_points * 3;
   sTessBaseData.m_pdCoords = (A3DDouble*)malloc(sTessBaseData.m_uiCoordSize * sizeof(A3DDouble));

   for (p = 0; p < num_points; p++)
   {
      sTessBaseData.m_pdCoords[p * 3] = points[p][0];
      sTessBaseData.m_pdCoords[p * 3 + 1] = points[p][1];
      sTessBaseData.m_pdCoords[p * 3 + 2] = points[p][2];
   }
   free(points);

   //---------------------------------------------------------------------
   // 3) Create A3DTess3DData - indices and normals
   //---------------------------------------------------------------------
   A3D_INITIALIZE_DATA(A3DTess3DData, sTess3DData);

   // 3 vectors of 3 doubles for each triangle
   sTess3DData.m_uiNormalSize = 9 * num_triangles;
   sTess3DData.m_pdNormals = (A3DDouble*)malloc(sTess3DData.m_uiNormalSize * sizeof(A3DDouble));
   // kA3DTessFaceDataTriangle uses 3 {normal, point} pairs for each triangle 
   // so there are 6 indices per triangle - see sTessFaceData.m_usUsedEntitiesFlags
   sTess3DData.m_uiTriangulatedIndexSize = 6 * num_triangles;
   sTess3DData.m_puiTriangulatedIndexes = (A3DUns32*)malloc(sTess3DData.m_uiTriangulatedIndexSize * sizeof(A3DUns32));
   // One A3DTessFaceData for each surface/CAD face
   sTess3DData.m_uiFaceTessSize = num_surfaces;
   sTess3DData.m_psFaceTessData = (A3DTessFaceData*)malloc(num_surfaces * sizeof(A3DTessFaceData));
   sTess3DData.m_bHasFaces = TRUE;

   vertex_index = 0;         // Index into indices and normals
   triangulated_index = 0;   // Index into m_puiTriangulatedIndexes
   normal_index = 0;         // Index into m_pdNormals
   for (t = 0; t < num_triangles; t++)
   {
      for (v = 0; v < 3; v++, vertex_index++)
      {
         // kA3DTessFaceDataTriangle uses a {normal, point} index pair for each vertex
         // The index into m_pdNormals indicates the first value (x) of the normal
         sTess3DData.m_puiTriangulatedIndexes[triangulated_index++] = normal_index;
         sTess3DData.m_puiTriangulatedIndexes[triangulated_index++] = indices[vertex_index] * 3;
         // Normal (x, y, z) for each vertex
         sTess3DData.m_pdNormals[normal_index++] = normals[vertex_index][0];
         sTess3DData.m_pdNormals[normal_index++] = normals[vertex_index][1];
         sTess3DData.m_pdNormals[normal_index++] = normals[vertex_index][2];
      }
   }
   free(indices);
   free(normals);

   //---------------------------------------------------------------------
   // 4) Create A3DTessFaceData - CAD face/surface data
   //---------------------------------------------------------------------
   first_surface_index = 0;
   for (s=0; s < num_surfaces; s++)
   {
      A3D_INITIALIZE_DATA(A3DTessFaceData, sTess3DData.m_psFaceTessData[s]);

      sTess3DData.m_psFaceTessData[s].m_usUsedEntitiesFlags = kA3DTessFaceDataTriangle;
      // Size of m_puiSizesTriangulated array
      sTess3DData.m_psFaceTessData[s].m_uiSizesTriangulatedSize = 1;
      // One size per polygon
      sTess3DData.m_psFaceTessData[s].m_puiSizesTriangulated = (A3DUns32*)malloc(sTess3DData.m_psFaceTessData[s].m_uiSizesTriangulatedSize * sizeof(A3DUns32));
      // Number of polygons in this face (NB not number of indices)
      sTess3DData.m_psFaceTessData[s].m_puiSizesTriangulated[0] = polygons_per_surface[s];
      sTess3DData.m_psFaceTessData[s].m_uiStartTriangulated = first_surface_index;
      // Increment the starting index by the number of indices in this face
      // (NB not the number of triangles)
      // Note also that there are 6 indices per polygon (3 vertex, 3 normal)
      first_surface_index += polygons_per_surface[s] * 6;

      if (surface_colours[s] != NULL)
      {
         A3DUns32 uiStyleIndex = A3D_DEFAULT_STYLE_INDEX;
         if (create_color_style(surface_colours[s][0], surface_colours[s][1], surface_colours[s][2], uiStyleIndex))
         {
            sTess3DData.m_psFaceTessData[s].m_uiStyleIndexesSize = 1;
            sTess3DData.m_psFaceTessData[s].m_puiStyleIndexes = (A3DUns32*)malloc(sTess3DData.m_psFaceTessData[s].m_uiStyleIndexesSize * A3DUns32(sizeof(A3DUns32)));
            sTess3DData.m_psFaceTessData[s].m_puiStyleIndexes[0] = uiStyleIndex;
         }
      }
   }
   free(polygons_per_surface);
   free(surface_colours);

   //---------------------------------------------------------------------
   // 5) Create A3DTess3D - solid tessellation
   //---------------------------------------------------------------------
   *ppTess3D = NULL;
   A3DTess3DCreate(&sTess3DData, ppTess3D);
   A3DTessBaseSet(*ppTess3D, &sTessBaseData);

   //---------------------------------------------------------------------
   // Clean-up
   //---------------------------------------------------------------------

   // Free arrays
   free(sTessBaseData.m_pdCoords);
   free(sTess3DData.m_pdNormals);
   free(sTess3DData.m_puiTriangulatedIndexes);
   for (s = 0; s < num_surfaces; s++)
   {
      free(sTess3DData.m_psFaceTessData[s].m_puiSizesTriangulated);
   }
   //free(sTess3DData.m_pdTextureCoords);

   return A3D_SUCCESS;
}
//---create_tessellation-----------------------------------------------

INTERNAL A3DStatus create_poly_brep_model(A3DRiPolyBrepModel** ppPolyBREP, 
                                          A3DTess3D* pTess3D, 
                                          A3D_log_func logging_function = nullptr)
{
   // Create a poly brep model from a tessellation
   A3DStatus iRet = A3D_SUCCESS;
   A3DRiPolyBrepModelData sPolyBrepModelData;
   A3DRiRepresentationItemData sRiData;

   A3D_INITIALIZE_DATA(A3DRiPolyBrepModelData, sPolyBrepModelData);
   sPolyBrepModelData.m_bIsClosed = TRUE;
   CHECK_A3DSTATUS(A3DRiPolyBrepModelCreate(&sPolyBrepModelData, ppPolyBREP), 
                   logging_function, 
                   "create_poly_brep_model - A3DRiPolyBrepModelCreate");

   /* Assign the tessellation to the PolyBrepModel */
   A3D_INITIALIZE_DATA(A3DRiRepresentationItemData, sRiData);
   sRiData.m_pTessBase = pTess3D;
   sRiData.m_pCoordinateSystem = NULL;
   CHECK_A3DSTATUS(A3DRiRepresentationItemSet(*ppPolyBREP, &sRiData),
                   logging_function, 
                   "create_poly_brep_model - A3DRiRepresentationItemSet");

   return iRet;
}
//---create_poly_brep_model--------------------------------------------

INTERNAL A3DStatus create_part(A3DAsmPartDefinition** ppPartDef,
                               A3DRiRepresentationItem** ppRIs, A3DUns32 uiRepItemsSize, 
                               A3D_log_func logging_function = nullptr)
{
   // Create a part definition from a representation item 
   // (for example, a A3DRiPolyBrepModel)
   A3DStatus iRet = A3D_SUCCESS;
   A3DAsmPartDefinitionData sPartDefinitionData;

   A3D_INITIALIZE_DATA(A3DAsmPartDefinitionData, sPartDefinitionData);
   sPartDefinitionData.m_uiRepItemsSize = uiRepItemsSize;
   sPartDefinitionData.m_ppRepItems = ppRIs;
   sPartDefinitionData.m_uiAnnotationsSize = 0;
   CHECK_A3DSTATUS(A3DAsmPartDefinitionCreate(&sPartDefinitionData, ppPartDef), 
                   logging_function, 
                   "create_part - A3DAsmPartDefinitionCreate");

   return iRet;
}
//---create_part-------------------------------------------------------

INTERNAL A3DStatus create_product_occurrence(A3DAsmProductOccurrence** product_occurrence,
                                             A3DAsmPartDefinition* part_definition,
                                             A3D_log_func logging_function = nullptr)
{
   // Create a product occurrence from a part definition
   A3DStatus iRet = A3D_SUCCESS;
   A3DAsmProductOccurrenceData sPOData;

   *product_occurrence = NULL;
   A3D_INITIALIZE_DATA(A3DAsmProductOccurrenceData, sPOData);
   sPOData.m_pPart = part_definition;
   CHECK_A3DSTATUS(A3DAsmProductOccurrenceCreate(&sPOData, product_occurrence),
                   logging_function,
                   "create_product_occurrence - A3DAsmProductOccurrenceCreate");

   return iRet;
}
//---create_product_occurrence-----------------------------------------

INTERNAL A3DStatus create_rgb_color(A3DUns32& uiIndexRgbColor, 
                                    A3DDouble dRed, A3DDouble dGreen, A3DDouble dBlue)
{
   A3DStatus iRet = A3D_SUCCESS;
   A3DGraphRgbColorData sData;
   A3D_INITIALIZE_DATA(A3DGraphRgbColorData, sData);
   sData.m_dRed = dRed;
   sData.m_dGreen = dGreen;
   sData.m_dBlue = dBlue;
   iRet = A3DGlobalInsertGraphRgbColor(&sData, &uiIndexRgbColor);

   return iRet;
}
//---create_rgb_color--------------------------------------------------

INTERNAL A3DStatus set_graphics_color(A3DRootBaseWithGraphicsData* pOutRootBaseWithGraphics, 
                                      A3DDouble dRed, A3DDouble dGreen, A3DDouble dBlue, A3DDouble dAlpha)
{
   A3DStatus iRet = A3D_SUCCESS;

   //Create a style color
   A3DUns32 uiColorIndex = 0;
   iRet = create_rgb_color(uiColorIndex, dRed, dGreen, dBlue);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   A3DUns32 uiStyleIndex = 0;
   A3DGraphStyleData sStyleData;
   A3D_INITIALIZE_DATA(A3DGraphStyleData, sStyleData);
   sStyleData.m_bMaterial = false;
   sStyleData.m_bVPicture = false;
   sStyleData.m_dWidth = 0.1; // default
   sStyleData.m_bIsTransparencyDefined = true;
   sStyleData.m_ucTransparency = (A3DUns8)(dAlpha * 255.0);
   sStyleData.m_bSpecialCulling = false;
   sStyleData.m_bBackCulling = false;
   sStyleData.m_uiRgbColorIndex = uiColorIndex;
   iRet = A3DGlobalInsertGraphStyle(&sStyleData, &uiStyleIndex);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   A3DGraphicsData sGraphicsData;
   A3D_INITIALIZE_DATA(A3DGraphicsData, sGraphicsData);

   sGraphicsData.m_uiStyleIndex = uiStyleIndex;
   sGraphicsData.m_usBehaviour = kA3DGraphicsShow;
   sGraphicsData.m_usBehaviour |= kA3DGraphicsSonHeritColor;

   iRet = A3DGraphicsCreate(&sGraphicsData, &(pOutRootBaseWithGraphics->m_pGraphics));

   return iRet;
}
//---set_graphics_color------------------------------------------------

INTERNAL A3DStatus set_entity_color(A3DEntity* inEntity, 
                                    A3DDouble dRed, A3DDouble dGreen, A3DDouble dBlue, A3DDouble dAlpha)
{
   A3DStatus iRet = A3D_SUCCESS;

   A3DRootBaseWithGraphicsData sBaseWithGraphicsData;
   A3D_INITIALIZE_DATA(A3DRootBaseWithGraphicsData, sBaseWithGraphicsData);
   iRet = A3DRootBaseWithGraphicsGet(inEntity, &sBaseWithGraphicsData);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   iRet = set_graphics_color(&sBaseWithGraphicsData, dRed, dGreen, dBlue, dAlpha);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   iRet = A3DRootBaseWithGraphicsSet(inEntity, &sBaseWithGraphicsData);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   iRet = A3DGraphicsDelete(sBaseWithGraphicsData.m_pGraphics);

   return iRet;
}
//---set_entity_color--------------------------------------------------

INTERNAL A3DStatus create_instance_product_occurrence(A3DAsmProductOccurrence** product_occurrence,
                                                      A3DAsmProductOccurrence* prototype_PO,
                                                      A3DMiscTransformation *location,
                                                      A3DBool set_color,
                                                      A3DDouble dRed, A3DDouble dGreen, A3DDouble dBlue, A3DDouble dAlpha,
                                                      A3D_log_func logging_function = nullptr)
{
   // Create a product occurrence from a part definition
   A3DStatus iRet = A3D_SUCCESS;
   A3DAsmProductOccurrenceData sPOData;

   *product_occurrence = NULL;
   A3D_INITIALIZE_DATA(A3DAsmProductOccurrenceData, sPOData);
   sPOData.m_pPrototype = prototype_PO;
   sPOData.m_pLocation = location;
   CHECK_A3DSTATUS(A3DAsmProductOccurrenceCreate(&sPOData, product_occurrence),
                                                 logging_function,
                                                 "create_product_occurrence - A3DAsmPartDefinitionCreate");

   if (set_color)
   {
      CHECK_A3DSTATUS(set_entity_color(*product_occurrence, dRed, dGreen, dBlue, dAlpha),
                                       logging_function,
                                       "create_product_occurrence - set_entity_color");
   }

   return iRet;
}
//---create_instance_product_occurrence--------------------------------

INTERNAL A3DStatus create_model_file(A3DAsmModelFile** model_file,
                                     A3DAsmProductOccurrence** product_occurrences,
                                     A3DUns32 num_product_occurrences,
                                     A3D_log_func logging_function = nullptr)
{
   A3DStatus iRet = A3D_SUCCESS;
   A3DAsmModelFileData sData;

   // Put everything under a 'root product occurrence'
   A3DAsmProductOccurrenceData sPOData;
   A3DAsmProductOccurrence* pRootPO = NULL;
   A3D_INITIALIZE_DATA(A3DAsmProductOccurrenceData, sPOData);
   sPOData.m_uiPOccurrencesSize = num_product_occurrences;
   sPOData.m_ppPOccurrences = product_occurrences;
   CHECK_A3DSTATUS(A3DAsmProductOccurrenceCreate(&sPOData, &pRootPO),
                   logging_function,
                   "create_model_file - A3DAsmProductOccurrenceCreate");

   A3D_INITIALIZE_DATA(A3DAsmModelFileData, sData);
   sData.m_uiPOccurrencesSize = 1;
   sData.m_dUnit = 1.0;
   sData.m_ppPOccurrences = &pRootPO;
   CHECK_A3DSTATUS(A3DAsmModelFileCreate(&sData, model_file),
                   logging_function,
                   "createModelFile - A3DAsmModelFileCreate");

   /*
   A3D_INITIALIZE_DATA(A3DAsmModelFileData, sData);
   sData.m_uiPOccurrencesSize = num_product_occurrences;
   sData.m_dUnit = 1.0;
   sData.m_ppPOccurrences = product_occurrences;
   CHECK_A3DSTATUS(A3DAsmModelFileCreate(&sData, model_file),
                   logging_function,
                   "createModelFile - A3DAsmModelFileCreate");
   */

   return iRet;
}
//---create_model_file-------------------------------------------------

/*!
\brief Creates an A3D model file from the provided Polygonica PTSolid
\param solid The Polygonica solid
\param ppModelFile [out] The model file containing a A3DRiPolyBrepModel
\return A3D_SUCCESS - Operation succeeded,
A3D_PG_ERROR - Internal polygonica error,
A3D error code - Internal A3D error
*/
INTERNAL int PGSolidCreateA3DModel(PTSolid solid, 
                                   A3DAsmModelFile** ppModelFile,
                                   A3D_log_func logging_function = nullptr)
{
   A3DStatus iRet = A3D_SUCCESS;
   int error;
   A3DTess3D* tessellation = NULL;
   A3DRiPolyBrepModel* poly_brep_model = NULL;
   A3DAsmPartDefinition* part_definition = NULL;
   A3DAsmProductOccurrence* product_occurrence = NULL;

   error = create_tessellation(&tessellation, solid);
   if (error != A3D_SUCCESS)
   {
      return error;
   }

   iRet = create_poly_brep_model(&poly_brep_model, tessellation);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   iRet = create_part(&part_definition, &poly_brep_model, 1);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   iRet = create_product_occurrence(&product_occurrence, part_definition);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }
   
   *ppModelFile = NULL;
   iRet = create_model_file(ppModelFile, &product_occurrence, 1);

   return iRet;
}
//---PGSolidCreateA3DModel---------------------------------------------

INTERNAL int update_rep_item_set(A3DPolygonicaOptions& pgOpts,
                                 A3DRiSet* pSet,
                                 A3D_log_func logging_function = nullptr);

INTERNAL int update_rep_item_array(A3DPolygonicaOptions& pgOpts, 
                                   A3DRiRepresentationItem** ppRepItems, 
                                   A3DUns32 uiRepItemsSize,
                                   A3D_log_func logging_function = nullptr)
{
   // Update the representation items in the array 
   // with new tessellations from Polygonica solids
   // NB A representation item may be a set, if so, traverse the set
   A3DStatus iRet = A3D_SUCCESS;
   A3DUns32 ui;
   A3DEEntityType eType;
   A3DRootBaseData sRootBaseData;
   A3DRootBaseWithGraphicsData sRootBaseGraphicsData;
   PTSolid solid = PV_ENTITY_NULL;
   A3DTess3D* pTess3D = NULL;
   A3DRiPolyBrepModel* pPolyBREP = NULL;
   int error;

   for (ui = 0; ui < uiRepItemsSize; ++ui)
   {
      A3DRiRepresentationItem* pRepItem = ppRepItems[ui];

      // Get the type of the representation item
      CHECK_A3DSTATUS(A3DEntityGetType(pRepItem, &eType),
                      logging_function, "update_rep_item_array - A3DEntityGetType");

      // Get the base data (name, attributes) of the representation item
      A3D_INITIALIZE_DATA(A3DRootBaseData, sRootBaseData);
      CHECK_A3DSTATUS(A3DRootBaseGet(pRepItem, &sRootBaseData),
                      logging_function, "update_rep_item_array - A3DRootBaseGet");

      // Get the graphics data of the representation item
      A3D_INITIALIZE_DATA(A3DRootBaseWithGraphicsData, sRootBaseGraphicsData);
      CHECK_A3DSTATUS(A3DRootBaseWithGraphicsGet(pRepItem, &sRootBaseGraphicsData),
                      logging_function, "update_rep_item_array - A3DRootBaseWithGraphicsGet");

      switch (eType)
      {
         case kA3DTypeRiSet:
         {
            iRet = (A3DStatus)update_rep_item_set(pgOpts, pRepItem, logging_function);

            break;
         }
         case kA3DTypeRiBrepModel:
         case kA3DTypeRiPolyBrepModel:
         {
            // Retrieve the PTSolid from the A3DPolygonicaOptions struct 
            // created on importing the model
            solid = PV_ENTITY_NULL;
            solid = pgOpts.m_parts[pRepItem];
            if (solid == PV_ENTITY_NULL)
            {
               break;
            }

            // Create a new tessellation from the solid
            error = create_tessellation(&pTess3D, solid);
            if (error != A3D_SUCCESS)
            {
               return error;
            }

            // Create a new poly BREP representation item
            create_poly_brep_model(&pPolyBREP, pTess3D, logging_function);

            // Set the base data (name, attributes) from the original representation item
            CHECK_A3DSTATUS(A3DRootBaseSet(pPolyBREP, &sRootBaseData),
                            logging_function, "update_rep_item_array - A3DRootBaseSet");

            // Set the graphics data from the original representation item
            CHECK_A3DSTATUS(A3DRootBaseWithGraphicsSet(pPolyBREP, &sRootBaseGraphicsData),
                            logging_function, "update_rep_item_array - A3DRootBaseWithGraphicsSet");

            // TO DO - Do we need somehow to free the original representation item?
            // This needs to be done without destroying the attributes or graphics data 
            // we have just copied to the new pPolyBREP item

            // Replace the existing item in the array
            ppRepItems[ui] = pPolyBREP;

            break;
         }
         default:
         {
         }
      }

      // Free data
      A3DRootBaseGet(NULL, &sRootBaseData);
      A3DRootBaseWithGraphicsGet(NULL, &sRootBaseGraphicsData);
   }

   return iRet;
}
//---update_rep_item_array---------------------------------------------

INTERNAL int update_rep_item_set(A3DPolygonicaOptions& pgOpts,
                                 A3DRiSet* pSet,
                                 A3D_log_func logging_function)
{
   // Traverse the representation item set, updating representation items
   // with new tessellations from Polygonica solids
   A3DStatus iRet = A3D_SUCCESS;
   A3DRiSetData sData;
   int error;

   A3D_INITIALIZE_DATA(A3DRiSetData, sData);
   iRet = A3DRiSetGet(pSet, &sData);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   // Update all representation items in the set
   error = update_rep_item_array(pgOpts, sData.m_ppRepItems, sData.m_uiRepItemsSize, logging_function);

   // Update the set with the altered array
   A3DRiSetEdit(&sData, pSet);

   // Free data
   A3DRiSetGet(NULL, &sData);

   return error;
}
//---update_rep_item_set-----------------------------------------------

INTERNAL int update_part_def(A3DPolygonicaOptions& pgOpts,
                             A3DAsmPartDefinition* pPart,
                             A3D_log_func logging_function = nullptr)
{
   // Traverse the part definition, updating representation items
   // with new tessellations from Polygonica solids
   A3DStatus iRet = A3D_SUCCESS;
   A3DAsmPartDefinitionData sData;
   int error;

   A3D_INITIALIZE_DATA(A3DAsmPartDefinitionData, sData);
   iRet = A3DAsmPartDefinitionGet(pPart, &sData);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   // Update all representation items in the part
   error = update_rep_item_array(pgOpts, sData.m_ppRepItems, sData.m_uiRepItemsSize, logging_function);

   // Update the part with the altered array
   A3DAsmPartDefinitionEdit(&sData, pPart);

   return error;
}
//---update_part_def---------------------------------------------------

INTERNAL int update_product_occurrence(A3DPolygonicaOptions& pgOpts,
                                       A3DAsmProductOccurrence* pOccurrence,
                                       A3D_log_func logging_function = nullptr)
{
   // Traverse the product occurrence, updating representation items 
   // with new tessellations from Polygonica solids
   A3DStatus iRet = A3D_SUCCESS;
   A3DUns32 ui;
   A3DAsmProductOccurrenceData sData;

   A3D_INITIALIZE_DATA(A3DAsmProductOccurrenceData, sData);
   iRet = A3DAsmProductOccurrenceGet(pOccurrence, &sData);

   if (sData.m_pPrototype)
   {
      update_product_occurrence(pgOpts, sData.m_pPrototype, logging_function);
   }
   else if (sData.m_pExternalData)
   {
      update_product_occurrence(pgOpts, sData.m_pExternalData, logging_function);
   }
   else
   {
      for (ui = 0; ui < sData.m_uiPOccurrencesSize; ++ui)
      {
         update_product_occurrence(pgOpts, sData.m_ppPOccurrences[ui], logging_function);
      }
   }

   if (sData.m_pPart)
   {
      update_part_def(pgOpts, sData.m_pPart, logging_function);
   }

   // Free data
   A3DAsmProductOccurrenceGet(NULL, &sData);

   return iRet;
}
//---update_product_occurrence-----------------------------------------

/*!
\brief Update a A3DAsmModelFile using Polygonica solids to create new tessellations.
This uses a A3DPolygonicaOptions structure created on import using A3DModelCreatePGWorld
to determine associations between PTSolids and A3DRiRepresentationItems.
\param pModelFile - A loaded model file to update. Should contain A3DRiPolyBrep or A3DRiBrepModel
\param pgOpts - Structure created on model import using A3DModelCreatePGWorld
\param logging_function - Optional logging function
\return A3D_SUCCESS - Operation succeeded,
A3D_PG_ERROR - Internal polygonica error,
A3D error code - Internal A3D error
*/
#if TRUE
// This version replaces all representation items with poly Brep items
INTERNAL int A3DModelUpdateFromPGWorld(A3DAsmModelFile* pModelFile,
                                       A3DPolygonicaOptions& pgOpts,
                                       A3D_log_func logging_function = nullptr)
{
   A3DStatus iRet = A3D_SUCCESS;
   A3DUns32 ui;
   A3DAsmModelFileData sData;

   A3D_INITIALIZE_DATA(A3DAsmModelFileData, sData);
   iRet = A3DAsmModelFileGet(pModelFile, &sData);
   if (iRet != A3D_SUCCESS)
   {
      return iRet;
   }

   // Traverse all product occurrences in the model, updating representation items
   for (ui = 0; ui < sData.m_uiPOccurrencesSize; ++ui)
   {
      update_product_occurrence(pgOpts, sData.m_ppPOccurrences[ui], logging_function);
   }

   // Free data
   A3DAsmModelFileGet(NULL, &sData);

   return iRet;
}
//---A3DModelUpdateFromPGWorld-----------------------------------------
#else
// This version sets a new tessellation for each representation item.
INTERNAL int A3DModelUpdateFromPGWorld(A3DAsmModelFile* pModelFile,
                                       A3DPolygonicaOptions& pgOpts,
                                       A3D_log_func logging_function = nullptr)
{
   A3DStatus iRet = A3D_SUCCESS;
   A3DTess3D* pTess3D = NULL;
   A3DRiRepresentationItem* pRepItem = NULL;
   A3DRiRepresentationItemData sRepItemData;
   PTSolid solid = PV_ENTITY_NULL;
   int error;

   for (auto it = pgOpts.m_parts.begin(); it != pgOpts.m_parts.end(); it++)
   {
      pRepItem = (A3DRiRepresentationItem * )it->first;
      solid = (PTSolid)it->second;
      error = createTessellation(&pTess3D, solid);
      if (error)
      {
         return error;
      }
      A3D_INITIALIZE_DATA(A3DRiRepresentationItemData, sRepItemData);
      iRet = A3DRiRepresentationItemGet(pRepItem, &sRepItemData);
      if (iRet != A3D_SUCCESS)
      {
         return iRet;
      }
      // TO DO - free the existing tessellation sRepItemData.m_pTessBase (not sure how to do this, unless it is done automatically in A3DRiRepresentationItemSet)
      sRepItemData.m_pTessBase = pTess3D;
      iRet = A3DRiRepresentationItemSet(pRepItem, &sRepItemData);
      if (iRet != A3D_SUCCESS)
      {
         return iRet;
      }
      A3DRiRepresentationItemGet(NULL, &sRepItemData);
   }
   return iRet;
}
//---A3DModelUpdateFromPGWorld-----------------------------------------
#endif

INTERNAL PTNat32 find_in_list(PTEntity entity, PTEntityList list)
{
   // Return the index of the entity in the list
   // or -1 if not found
   PTEntity list_entity = PV_ENTITY_NULL;
   PTNat32 index;
   if (list == PV_ENTITY_NULL)
   {
      return -1;
   }

   for (index = 0, list_entity = PFEntityListGetFirst(list);
        list_entity;
        list_entity = PFEntityListGetNext(list, list_entity), index++)
   {
      if (list_entity == entity)
      {
         return index;
      }
   }

   return -1;
}
//---find_in_list------------------------------------------------------

INTERNAL PTStatus list_unique_solids(PTEnvironment env,
                                     PTEntityList world_entities,
                                     PTEntityList* solids_out)
{
   // Given a list of world entities, return a list of unique entities (solid/curve) 
   // from which they were derived
   PTStatus status = PV_STATUS_OK;
   PTEntity entity = PV_ENTITY_NULL;
   PTEntityList solids = PV_ENTITY_NULL;
   PTWorldEntity world_entity = PV_ENTITY_NULL;

   // Create an entity list to store entities
   status = PFEntityListCreate(env, NULL, 0, NULL, &solids);
   if (status != PV_STATUS_OK)
   {
      return status;
   }

   for (world_entity = PFEntityListGetFirst(world_entities);
        world_entity != PV_ENTITY_NULL;
        world_entity = PFEntityListGetNext(world_entities, world_entity))
   {
      // Get the PTEntity from which the PTWorldEntity was derived
      entity = PFEntityGetEntityProperty(world_entity, PV_WENTITY_PROP_ENTITY);
      if (entity == PV_ENTITY_NULL)
      {
         continue;
      }
      if (PFEntityGetEnumProperty(entity, PV_ENTITY_PROP_TYPE) != PV_ENTITY_TYPE_SOLID)
      {
         continue;
      }

      // Check whether the solid has already been added to the list
      if (find_in_list(entity, solids) != -1)
      {
         continue;
      }

      // Append the solid to the list
      status = PFEntityListAppend(solids, entity);
      if (status != PV_STATUS_OK)
      {
         goto cleanup;
      }

   }

cleanup:

   if (status != PV_STATUS_OK)
   {
      // TO DO: delete any entries on the list
      PFEntityListDestroy(solids, 0);
   }
   else
   {
      // Set output list before returning
      *solids_out = solids;
   }

   return status;
}
//---list_unique_solids------------------------------------------------

INTERNAL void create_transform(PTTransformMatrix transform, 
                               A3DMiscTransformation** pTransformation)
{
   // Create an A3DMiscTransformation * from a Polygonica PTTransformMatrix
   A3DMiscGeneralTransformationData sTransformData;
   A3D_INITIALIZE_DATA(A3DMiscGeneralTransformationData, sTransformData);
   for (int i = 0; i < 4; i++)
   {
      for (int j = 0; j < 4; j++)
      {
         sTransformData.m_adCoeff[(i * 4) + j] = transform[i][j];
      }
   }
   *pTransformation = NULL;
   A3DMiscGeneralTransformationCreate(&sTransformData, pTransformation);
}
//---create_transform--------------------------------------------------

INTERNAL A3DBool product_occurrence_has_face_colours(A3DAsmProductOccurrence* pPO)
{
   // Return TRUE if the product occurrence contains
   // a part, which contains
   // a representation item, which contains
   // a tessellation, which contains at least one CAD face, which has
   // a valid style, which represents
   // a non-default colour
   A3DAsmProductOccurrenceData sPOData;
   A3DAsmPartDefinitionData    sPartData;
   A3DRiRepresentationItemData sRiData;
   A3DTess3DData               sTessData;
   A3DGraphStyleData           sStyleData;
   A3DBool                     valid_style;
   A3DUns32                    uiRgbColorIndex;
   A3DGraphRgbColorData        sColorData;

   A3D_INITIALIZE_DATA(A3DAsmProductOccurrenceData, sPOData);
   A3DAsmProductOccurrenceGet(pPO, &sPOData);
   if (sPOData.m_pPart == NULL)
   {
      A3DAsmProductOccurrenceGet(NULL, &sPOData);
      return FALSE;
   }

   A3D_INITIALIZE_DATA(A3DAsmPartDefinitionData, sPartData);
   A3DAsmPartDefinitionGet(sPOData.m_pPart, &sPartData);
   A3DAsmProductOccurrenceGet(NULL, &sPOData);
   if (sPartData.m_uiRepItemsSize < 1)
   {
      A3DAsmPartDefinitionGet(NULL, &sPartData);
      return FALSE;
   }

   A3D_INITIALIZE_DATA(A3DRiRepresentationItemData, sRiData);
   A3DRiRepresentationItemGet(sPartData.m_ppRepItems[0], &sRiData);
   A3DAsmPartDefinitionGet(NULL, &sPartData);

   A3D_INITIALIZE_DATA(A3DTess3DData, sTessData);
   A3DTess3DGet(sRiData.m_pTessBase, &sTessData);
   A3DRiRepresentationItemGet(NULL, &sRiData);
   if (sTessData.m_uiFaceTessSize < 1)
   {
      A3DTess3DGet(NULL, &sTessData);
      return FALSE;
   }

   valid_style = get_face_style(sTessData.m_psFaceTessData[0], sStyleData);
   A3DTess3DGet(NULL, &sTessData);

   if (!valid_style)
   {
      return FALSE;
   }

   if (sStyleData.m_uiRgbColorIndex == A3D_DEFAULT_COLOR_INDEX)
   {
      return FALSE;
   }

   if (sStyleData.m_bMaterial == TRUE)
   {

      A3DBool isTexture = false;
      A3DGlobalIsMaterialTexture(sStyleData.m_uiRgbColorIndex, &isTexture);

      if (isTexture)
      {
         A3DGlobalGetGraphStyleData(A3D_DEFAULT_STYLE_INDEX, &sStyleData);
         return FALSE;
      }
      else // Material
      {
         A3DGraphMaterialData gmd;
         A3D_INITIALIZE_DATA(A3DGraphMaterialData, gmd);
         A3DGlobalGetGraphMaterialData(sStyleData.m_uiRgbColorIndex, &gmd);
         uiRgbColorIndex = gmd.m_uiDiffuse;
         A3DGlobalGetGraphMaterialData(A3D_DEFAULT_COLOR_INDEX, &gmd);
      }
   }
   else
   {
      uiRgbColorIndex = sStyleData.m_uiRgbColorIndex;
   }
   A3DGlobalGetGraphStyleData(A3D_DEFAULT_STYLE_INDEX, &sStyleData);
   if (uiRgbColorIndex == A3D_DEFAULT_COLOR_INDEX)
   {
      return FALSE;
   }

   A3D_INITIALIZE_DATA(A3DGraphRgbColorData, sColorData);
   if (A3DGlobalGetGraphRgbColorData(uiRgbColorIndex, &sColorData) == A3D_SUCCESS)
   {
      A3DGlobalGetGraphRgbColorData(A3D_DEFAULT_COLOR_INDEX, &sColorData);
      return TRUE;
   }

   return FALSE;
}
//---product_occurrence_has_face_colours-------------------------------

/*!
\brief Creates an A3D model file from the provided Polygonica PTWorld
\param world The Polygonica world
\param ppModelFile [out] The model file containing an assembly representing the world
\param bridge_data Data created by loading a model through the bridge (may be NULL)
\return A3D_SUCCESS - Operation succeeded,
A3D_PG_ERROR - Internal polygonica error,
A3D error code - Internal A3D error
*/
INTERNAL int PGWorldCreateA3DModel(PTEnvironment env, PTWorld world,
                                   A3DPolygonicaOptions* bridge_data,
                                   A3DAsmModelFile** ppModelFile,
                                   A3D_log_func logging_function = nullptr)
{
   A3DStatus                     A3D_status = A3D_SUCCESS;
   A3DTess3D*                    tessellation = NULL;
   A3DRiPolyBrepModel*           poly_brep_model = NULL;
   A3DAsmPartDefinition*         part_definition = NULL;
   A3DAsmProductOccurrence*      prototype_PO = NULL;
   A3DAsmProductOccurrence*      instance_PO = NULL;
   A3DAsmProductOccurrence**     prototype_POs = NULL;
   A3DAsmProductOccurrence**     instance_POs = NULL;
   A3DMiscGeneralTransformation* location = NULL;

   PTStatus                      PG_status = PV_STATUS_OK;
   PTEntityList                  world_entities = PV_ENTITY_NULL;
   PTWorldEntity                 world_entity = PV_ENTITY_NULL;
   PTEntityList                  solids = PV_ENTITY_NULL;
   PTSolid                       solid = PV_ENTITY_NULL;
   PTNat32                       num_world_entities = 0;
   PTNat32                       num_solids = 0;
   PTNat32                       prototype_index = 0;
   PTTransformMatrix             transform;
   double                        colour[3];
   PTBoolean                     valid_colour;

   int                           error = A3D_SUCCESS;

   // Create a list of all world entities in the world
   PG_status = PFEntityCreateEntityList(world, PV_ENTITY_TYPE_WORLD_ENTITY, NULL, &world_entities);
   if (PG_status != PV_STATUS_OK)
   {
      goto cleanup;
   }
   num_world_entities = PFEntityGetNat32Property(world_entities, PV_ELIST_PROP_NUM_ENTITIES);
   // Create a list of all solids in the world
   PG_status = list_unique_solids(env, world_entities, &solids);
   if (PG_status != PV_STATUS_OK)
   {
      goto cleanup;
   }
   num_solids = PFEntityGetNat32Property(solids, PV_ELIST_PROP_NUM_ENTITIES);

   prototype_POs = (A3DAsmProductOccurrence**)malloc(sizeof(A3DAsmProductOccurrence*) * num_solids);
   instance_POs = (A3DAsmProductOccurrence**)malloc(sizeof(A3DAsmProductOccurrence*) * num_world_entities);

   // For each solid, create a prototype product occurrence
   // containing a part definition with a poly BREP representation item
   num_solids = 0;
   for (solid = PFEntityListGetFirst(solids);
        solid != PV_ENTITY_NULL;
        solid = PFEntityListGetNext(solids, solid))
   {
      error = create_tessellation(&tessellation, solid, logging_function);
      if (error != A3D_SUCCESS)
      {
         goto cleanup;
      }

      A3D_status = create_poly_brep_model(&poly_brep_model, tessellation, 
                                          logging_function);
      if (A3D_status != A3D_SUCCESS)
      {
         goto cleanup;
      }

      A3D_status = create_part(&part_definition, &poly_brep_model, 1, 
                               logging_function);
      if (A3D_status != A3D_SUCCESS)
      {
         goto cleanup;
      }

      // Create a prototype product occurrence with part definition
      A3D_status = create_product_occurrence(&prototype_PO, part_definition, 
                                             logging_function);
      if (A3D_status != A3D_SUCCESS)
      {
         goto cleanup;
      }

      prototype_POs[num_solids++] = prototype_PO;
   }

   // For each world entity, create an instance product occurrence
   // containing a position and a reference to a prototype
   num_world_entities = 0;
   for (world_entity = PFEntityListGetFirst(world_entities);
        world_entity != PV_ENTITY_NULL;
        world_entity = PFEntityListGetNext(world_entities, world_entity))
   {
      // Get the prototype for the instance
      // this being the product occurrence containing the 
      // representation item of the world entity's solid
      solid = PFEntityGetEntityProperty(world_entity, PV_WENTITY_PROP_ENTITY);
      if (solid == PV_ENTITY_NULL)
      {
         continue;
      }
      if (PFEntityGetEnumProperty(solid, PV_ENTITY_PROP_TYPE) != PV_ENTITY_TYPE_SOLID)
      {
         continue;
      }
      prototype_index = find_in_list(solid, solids);
      if (prototype_index < 0)
      {
         continue;
      }
      prototype_PO = prototype_POs[prototype_index];

      // Get the transform for the instance
      PG_status = PFWorldEntityGetTransform(world_entity, transform);
      create_transform(transform, &location);

      // Get the colour of the instance
      if (bridge_data == NULL)
      {
#ifdef PGRENDER_HDR
         // Get the colour from the render style of the world entity
         valid_colour = get_colour_from_render_style(world_entity, colour);
#else
         valid_colour = FALSE;
#endif
      }
      else
      {
         // Get the colour from the colour map in bridge_data
         valid_colour = get_colour_from_map(world_entity,
                                            bridge_data->m_entity_colours, colour);
      }
      if (valid_colour &&
          product_occurrence_has_face_colours(prototype_PO))
      {
         valid_colour = FALSE;
      }

      // Create an instance product occurrence with prototype and position
      // but no part definition
      A3D_status = create_instance_product_occurrence(&instance_PO, 
                                                      prototype_PO, location,
                                                      (A3DBool)valid_colour,
                                                      colour[0], colour[1], colour[2], 1.0,
                                                      logging_function);
      if (A3D_status != A3D_SUCCESS)
      {
         goto cleanup;
      }

      instance_POs[num_world_entities++] = instance_PO;
   }

   // Create a model file containing the instance product occurrences
   *ppModelFile = NULL;
   A3D_status = create_model_file(ppModelFile, instance_POs, num_world_entities, 
                                  logging_function);

cleanup:

   if (solids != PV_ENTITY_NULL)
   {
      PFEntityListDestroy(solids, 0);
   }
   if (world_entities != PV_ENTITY_NULL)
   {
      PFEntityListDestroy(world_entities, 0);
   }
   if (prototype_POs != NULL)
   {
      free(prototype_POs);
   }
   if (instance_POs != NULL)
   {
      free(instance_POs);
   }

   return A3D_status;
}
//---PGWorldCreateA3DModel---------------------------------------------

INTERNAL int PGWorldCreateA3DModel(PTEnvironment env, PTWorld world,
                                   A3DAsmModelFile** ppModelFile,
                                   A3D_log_func logging_function = nullptr)
{
   // A version of the preceding function not using a colour map
   return PGWorldCreateA3DModel(env, world, NULL,
                                ppModelFile,
                                logging_function);
}
//---PGWorldCreateA3DModel---------------------------------------------

INTERNAL PTStatus A3DTopoFaceQuery(A3DTopoFace *pFace, A3DUns8 orientation, A3DDouble scale,
                                   PTPoint point, PTPoint closest_point, PTVector normal)
{
   // Return the closest point on the A3DTopoFace 
   // and the normal to the surface at that point 
   // orientation is the orientation of the face with its containing shell 
   // (a value of 0 indicates face and shell have opposite orientations) 
   // scale is the native scale of the underlying modelling system 
   // which the CAD system used which generated the CAD file. Some work in mm others meters. 
   // Returns PV_SURF_QUERY_STATUS_NO_NORMAL | PV_SURF_QUERY_STATUS_NO_POINT on failure

   PTStatus return_code = PV_SURF_QUERY_STATUS_NO_NORMAL | PV_SURF_QUERY_STATUS_NO_POINT;
   // If PV_SURF_QUERY_STATUS_ERROR is returned, an undefined error has occurred, and remeshing will exit

   A3DTopoFaceData sData;
   A3D_INITIALIZE_DATA(A3DTopoFaceData, sData);

   A3DInt32 iRet = A3DTopoFaceGet(pFace, &sData);
   if (iRet != A3D_SUCCESS)
   {
      return return_code;
   }

   A3DSurfBase* surf = sData.m_pSurface;
   A3DVector3dData pt;
   A3DUns32 nSolutions = 0;
   A3DVector2dData* vec2dData;
   A3DDouble* solutionDistanceData;
   A3D_INITIALIZE_DATA(A3DVector3dData, pt);

   // Convert from model units to m
   pt.m_dX = point[0] / scale;
   pt.m_dY = point[1] / scale;
   pt.m_dZ = point[2] / scale;

   // Project the point onto the surface to get U/V coordinates
   iRet = A3DSurfProjectPoint(surf, &pt, &nSolutions, &vec2dData, &solutionDistanceData);
   if (iRet == A3D_SUCCESS)
   {
      if (nSolutions > 0)
      {
         // Choose the solution with the minimum distance (if nSolutions>1)
         A3DDouble minDistance = solutionDistanceData[0];
         A3DUns32 minIndex = 0;
         for (A3DUns32 i = 1; i < nSolutions; i++)
         {
            if (solutionDistanceData[i] < minDistance)
            {
               minDistance = solutionDistanceData[i];
               minIndex = i;
            }
         }

         // Evaluate the U/V coordinates
         // to retrieve a 3D point on the surface
         A3DVector3dData pointAndDerivatives;
         A3D_INITIALIZE_DATA(A3DVector3dData, pointAndDerivatives);
         iRet = A3DSurfEvaluate(surf, (const A3DVector2dData*)&(vec2dData[minIndex]), 0, &pointAndDerivatives);
         if (iRet == A3D_SUCCESS)
         {
            // Convert back to model units
            closest_point[0] = pointAndDerivatives.m_dX * scale;
            closest_point[1] = pointAndDerivatives.m_dY * scale;
            closest_point[2] = pointAndDerivatives.m_dZ * scale;
            return_code &= ~PV_SURF_QUERY_STATUS_NO_POINT;

            // Retrieve the surface normal at the point
            A3DVector3dData surfNormal;
            A3D_INITIALIZE_DATA(A3DVector3dData, surfNormal);
            iRet = A3DSurfEvaluateNormal(surf, (const A3DVector2dData*)&(vec2dData[minIndex]), &surfNormal);
            if (iRet == A3D_SUCCESS)
            {
               normal[0] = surfNormal.m_dX;
               normal[1] = surfNormal.m_dY;
               normal[2] = surfNormal.m_dZ;

               // Check the orientation of the face within the shell
               // A value of 0 indicates face and shell have opposite orientations 
               // (so we need to reverse the normal)
               if (orientation == 0)
               {
                  normal[0] = -normal[0];
                  normal[1] = -normal[1];
                  normal[2] = -normal[2];
               }

               return_code &= ~PV_SURF_QUERY_STATUS_NO_NORMAL;
            }
         }
      }

      // Free arrays
      A3DSurfProjectPoint(NULL, &pt, &nSolutions, &vec2dData, &solutionDistanceData);
   }

   // Free face data
   A3DTopoFaceGet(NULL, &sData);

   return return_code;
}
//---A3DTopoFaceQuery--------------------------------------------------

INTERNAL PTStatus PGAppSurfaceQuery(PTAppSurface app_surface,
                                    A3DPolygonicaOptions& pgOpts,
                                    PTPoint point,
                                    PTPoint closest_point,
                                    PTVector normal)
{
   // Return the closest point on the CAD surface indicated by the PTAppSurface 
   // and the normal to the surface at that point
   // The pgOpts structure is used to determine the CAD surface (A3DTopoFace and orientation) 
   // from the PTAppSurface
   // Returns PV_SURF_QUERY_STATUS_NO_NORMAL | PV_SURF_QUERY_STATUS_NO_POINT on failure

   A3DTopoFace* pFace = pgOpts.m_surface_faces[app_surface];
   A3DUns8 orientation;
   A3DDouble scale;

   if (pFace == NULL)
   {
      return PV_SURF_QUERY_STATUS_NO_NORMAL | PV_SURF_QUERY_STATUS_NO_POINT;
   }

   orientation = pgOpts.m_surface_face_orientations[app_surface];

   // Convert from model scale (m or mm) to m
   scale = pgOpts.m_scale;

   return A3DTopoFaceQuery(pFace, orientation, scale, point, closest_point, normal);
}
//---PGAppSurfaceQuery-------------------------------------------------
