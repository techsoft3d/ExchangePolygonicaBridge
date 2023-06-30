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
*
*      This software is provided "as is" without express or implied warranty, on an unsupported basis. 
*/
#pragma once

/*********************************************************************/
/***includes**********************************************************/
/*********************************************************************/

#include <A3DSDKIncludes.h>
#include "pg/pgapi.h"
#include "pg/pgrender.h"

#include <algorithm>
#include <unordered_map>
#include <map>
#include <string>

/*********************************************************************/
/***defines***********************************************************/
/*********************************************************************/

#define A3D_PG_NOT_INITIALIZED   1
#define A3D_PG_INVALID_RI        2
#define A3D_PG_ERROR             3

/* The use of 'static' is to provide support for older compilers that do not support 'inline' */
/* If you are using a modern compiler inline should probably be used */
#ifdef __cplusplus
#define INTERNAL inline
#else
#define INTERNAL static
#endif

#define TEST_RET(code) if (code)

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

/*********************************************************************/
/***structs***********************************************************/
/*********************************************************************/

enum A3D_log_level
{
   A3D_LOG_INFO,
   A3D_LOG_WARN,
   A3D_LOG_ERROR
};
/***A3D_log_level*****************************************************/

typedef void (*A3D_log_func)(std::string, A3D_log_level);

struct A3DPolygonicaOptions
{
   PTEnvironment m_Environment;
   PTWorld m_World;

   /* A mapping of A3DRiRepresentationItems to PTSolids*/
   std::unordered_map<const A3DRiRepresentationItem*, PTSolid> m_parts;
   /* A vector of PTWorldEntities */
   std::vector<PTWorldEntity> m_entities;
   /* A map providing one PTRenderStyle for each colour */
   std::map<unsigned long, PTRenderStyle> m_style_palette;
   /* A map providing a vector of groups of faces on each CAD surface for each PTSolid*/
   std::unordered_map<PTSolid, std::vector <PTEntityGroup>*> m_surface_groups;
   /* A map providing a vector part path for each PTWorldEntity */
   std::unordered_map<PTWorldEntity, std::vector<void*>*> m_paths;

   long m_iTopoFaceCount = 0;
};
/***A3DPolygonicaOptions**********************************************/

struct MiscCascadedAttributesGuard
{
   A3DMiscCascadedAttributes* ptr_;

   MiscCascadedAttributesGuard(A3DMiscCascadedAttributes* ptr) : ptr_(ptr)
   {}

   ~MiscCascadedAttributesGuard()
   {
      TEST_RET(A3DMiscCascadedAttributesDelete(ptr_))
      {
         ptr_ = NULL;
      }
   }
};
/***MiscCascadedAttributesGuard***************************************/

/*********************************************************************/
/***functions*********************************************************/
/*********************************************************************/

INTERNAL void CHECK_A3DSTATUS(A3DStatus status,
                              A3D_log_func logger, 
                              const char* operation_name)
{
   if (logger == nullptr || status == A3D_SUCCESS) return;
   auto error_code_message = std::string(A3DMiscGetErrorMsg(status));
   logger(error_code_message + " ( in: " + operation_name + ")", A3D_log_level::A3D_LOG_ERROR);
}
/***CHECK_A3DSTATUS***************************************************/

INTERNAL void CHECK_PTSTATUS(PTStatus status,
                             A3D_log_func logger, 
                             const char* operation_name)
{
   if (logger == nullptr || status == PV_STATUS_OK) return;
   logger("Polygonica error " + std::to_string(status) + " ( in: " + operation_name + ")", A3D_log_level::A3D_LOG_ERROR);
}
/***CHECK_PTSTATUS****************************************************/

INTERNAL void log(A3D_log_func logging_function,
                  std::string message, A3D_log_level level)
{
   if (logging_function == nullptr)
   {
      return;
   }
   logging_function(message, level);
}
/***log***************************************************************/

/*!
\brief Returns the name of the part or product occurance
\param pPartOrProduct The part or product occurance
\param name [out] The name
\return A3D_SUCCESS - Operation succeeded
*/
INTERNAL A3DStatus stGetName(const A3DRootBaseWithGraphics* pPartOrProduct,
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
/***stGetName*********************************************************/

INTERNAL A3DStatus stExtractColorFromGraphicData(const A3DRootBaseWithGraphics* pRootBaseWithGraphics,
                                                 const A3DGraphStyleData& sGraphStyleData,
                                                 float& r, float& g, float& b,
                                                 A3D_log_func logging_function)
{
   // Gets the color inherited color information
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
      r = (float)sColorData.m_dRed;
      g = (float)sColorData.m_dGreen;
      b = (float)sColorData.m_dBlue;
   }

   A3DGlobalGetGraphRgbColorData(A3D_DEFAULT_COLOR_INDEX, &sColorData);

   return A3D_SUCCESS;
}
/***stExtractColorFromGraphicData*************************************/

INTERNAL A3DStatus stCreateAndPushCascadedAttributes(const A3DRootBaseWithGraphics* pBase,
                                                     const A3DMiscCascadedAttributes* pFatherAttr,
                                                     A3DMiscCascadedAttributes** ppAttr,
                                                     A3DMiscCascadedAttributesData* psAttrData,
                                                     A3D_log_func logging_function)
{
   // Creates an A3DMiscCascadedAttributes structure and computes the attributes
   // based on the input A3DRootBaseWithGraphics structure pBase.
   CHECK_A3DSTATUS(A3DMiscCascadedAttributesCreate(ppAttr), logging_function, 
                   "stCreateAndPushCascadedAttributes - A3DMiscCascadedAttributesCreate");
   CHECK_A3DSTATUS(A3DMiscCascadedAttributesPush(*ppAttr, pBase, pFatherAttr), logging_function, 
                   "stCreateAndPushCascadedAttributes - A3DMiscCascadedAttributesPush");

   A3D_INITIALIZE_DATA(A3DMiscCascadedAttributesData, (*psAttrData));
   CHECK_A3DSTATUS(A3DMiscCascadedAttributesGet(*ppAttr, psAttrData), logging_function, 
                   "stCreateAndPushCascadedAttributes - A3DMiscCascadedAttributesGet");
   return A3D_SUCCESS;
}
/***stCreateAndPushCascadedAttributes*********************************/

INTERNAL A3DStatus IndicesPerFaceAsTriangles(A3DTess3DData sTessData,
                                             const unsigned& uFaceIndice,
                                             std::vector<unsigned>& auIndices,
                                             std::vector<PTInt32>& normal_indices,
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

            puiTriangulatedIndexes += pFaceTessData->m_uiTextureCoordIndexesSize;

            COPY_(auIndices, puiTriangulatedIndexes, 1);
            puiTriangulatedIndexes += 1;
         }
      }
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleFanTextured)
   {
      log(logging_function, 
          "IndicesPerFaceAsTriangles cannot parse textured triangle data kA3DTessFaceDataTriangleFanTextured", A3D_LOG_ERROR);
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
          "IndicesPerFaceAsTriangles cannot parse textured triangle data kA3DTessFaceDataTriangleOneNormalTextured", A3D_LOG_ERROR);
      return A3D_ERROR;
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleFanOneNormalTextured)
   {
      log(logging_function, 
          "IndicesPerFaceAsTriangles cannot parse textured triangle data kA3DTessFaceDataTriangleFanOneNormalTextured", A3D_LOG_ERROR);
      return A3D_ERROR;
   }

   if (pFaceTessData->m_usUsedEntitiesFlags & kA3DTessFaceDataTriangleStripeOneNormalTextured)
   {
      log(logging_function, 
          "IndicesPerFaceAsTriangles cannot parse textured triangle data kA3DTessFaceDataTriangleStripeOneNormalTextured", A3D_LOG_ERROR);
      return A3D_ERROR;
   }

   if (unprocessed_flags != 0x0000)
   {
      log(logging_function, 
          "IndicesPerFaceAsTriangles could not understand triangle data flag " + std::to_string(unprocessed_flags), A3D_LOG_ERROR);
      return A3D_ERROR; // there was data of a type not handled by this function.
   }

   if (uFaceIndice == sTessData.m_uiFaceTessSize - 1)
   {
      // If this is the last face, check that the triangle index data has been used up exactly
      if (puiTriangulatedIndexes != &sTessData.m_puiTriangulatedIndexes[sTessData.m_uiTriangulatedIndexSize])
      {
         log(logging_function, 
             "IndicesPerFaceAsTriangles did not use the same number of triangle indexes as the model", A3D_LOG_ERROR);
         return A3D_ERROR;
      }
   }

   return A3D_SUCCESS;
}
/***IndicesPerFaceAsTriangles*****************************************/

INTERNAL PTBoolean face_in_category_cb(PTCategory cat, PTFace face)
{
   // Category selection callback to include 
   // all faces with app_surface matching the category's app_data
   void* uFaceTopoFace = (void*)PFEntityGetPointerProperty(face, PV_FACE_PROP_APP_SURFACE);
   void* uCategoryTopoFace = (void*)PFEntityGetPointerProperty(cat, PV_SOLID_PROP_APP_DATA);

   return (uFaceTopoFace == uCategoryTopoFace);
}
/***face_in_category_cb***********************************************/

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
                                                  PTSolid* solid, 
                                                  A3DPolygonicaOptions* opts, 
                                                  A3D_log_func logging_function = nullptr)
{
   A3DEEntityType eType;
   CHECK_A3DSTATUS(A3DEntityGetType(ri, &eType), logging_function, "A3DRiRepresentationItemCreatePTSolid - failed to get type of representation item");
   if (eType != A3DEEntityType::kA3DTypeRiBrepModel && eType != A3DEEntityType::kA3DTypeRiPolyBrepModel) return A3D_PG_INVALID_RI;

   A3DStatus iRet = A3D_SUCCESS;
   PTStatus status = PV_ENTITY_NULL;

   A3DRiRepresentationItemData	sRiData;
   A3D_INITIALIZE_DATA(A3DRiRepresentationItemData, sRiData);
   CHECK_A3DSTATUS(A3DRiRepresentationItemGet(ri, &sRiData), logging_function, "A3DRiRepresentationItemCreatePTSolid - A3DRiRepresentationItemGet");

   A3DTess3DData sTessData;
   A3D_INITIALIZE_DATA(A3DTess3DData, sTessData);
   A3DTess3DGet(sRiData.m_pTessBase, &sTessData);

   A3DTessBaseData sBaseTessData;
   A3D_INITIALIZE_DATA(A3DTessBaseData, sBaseTessData);
   A3DTessBaseGet(sRiData.m_pTessBase, &sBaseTessData);

   // Get Indices and Normals
   std::vector<unsigned int> auIndices;
   std::vector<PTPointer> faceAppSurface;
   std::vector<double> normals;
   std::vector<PTInt32> normal_indices;

   unsigned uTopoFace, uFaceSize = sTessData.m_uiFaceTessSize, uFaceAppData = 0;
   for (uTopoFace = 0; uTopoFace < uFaceSize; uTopoFace++)
   {
      IndicesPerFaceAsTriangles(sTessData, uTopoFace, auIndices, normal_indices, logging_function);
      faceAppSurface.insert(faceAppSurface.end(), 
                            auIndices.size() / 3 - faceAppSurface.size(), 
                            (PTPointer)(PTNat64)(opts->m_iTopoFaceCount + uTopoFace));
   }

   for (int i = 0; i < auIndices.size(); i++)
   {
      auIndices[i] = auIndices[i] / 3;
   }

   PTMeshSolidOpts meshOpts;
   PMInitMeshSolidOpts(&meshOpts);

   for (int i = 0; i < normal_indices.size(); i++)
   {
      normal_indices[i] = normal_indices[i] / 3;
   }

   meshOpts.normals = (PTVector*)sTessData.m_pdNormals;
   meshOpts.normal_indices = normal_indices.data();

   meshOpts.app_surfaces = (PTPointer*)faceAppSurface.data();

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

   if (status == PV_STATUS_OK)
   {
      // Create a vector of PTEntityGroups containing faces on each topoFace
      std::vector <PTEntityGroup>* groups = new std::vector<PTEntityGroup>;
      for (int topoFace = 0; topoFace < (int)uFaceSize; topoFace++)
      {
         PTEntityGroup group;
         status = PFEntityGroupCreate(opts->m_Environment, &group);
         groups->push_back(group);
      }

      // Add each face to a surface (topoFace) group
      PTEntityList faces = PV_ENTITY_NULL;
      PFEntityCreateEntityList(*solid, PV_ENTITY_TYPE_FACE, NULL, &faces);
      long max_groups = (long)groups->size();
      for (PTEntity face = PFEntityListGetFirst(faces); face != PV_ENTITY_NULL; face = PFEntityListGetNext(faces, face))
      {
         // unsigned long long used to remove compiler warning. 
         auto app_surface_index = (long long)PFEntityGetPointerProperty(face, PV_FACE_PROP_APP_SURFACE);
         long uFaceTopoFace = (long)(app_surface_index - opts->m_iTopoFaceCount);
         if ((uFaceTopoFace < 0) || (uFaceTopoFace >= max_groups))
         {
            // Invalid app surface value
            logging_function("Invalid AppSurface retrieved from PTFace", A3D_log_level::A3D_LOG_ERROR);
            // Delete all groups rather than pass on invalid data
            // NULL group will be added to opts for this solid
            for (int topoFace = 0; topoFace < (int)uFaceSize; topoFace++)
            {
               PFEntityGroupDestroy((*groups)[topoFace]);
            }
            delete groups;
            groups = NULL;
            // Set a failure return code
            iRet = A3D_LOAD_INVALID_FILE_FORMAT;
            break;
         }
         else
         {
            PFEntityGroupAddEntity((*groups)[uFaceTopoFace], face);
         }
      }
      PFEntityListDestroy(faces, 0);

      // Add a solid / group vector pair to the output map m_surface_groups
      opts->m_surface_groups.insert(std::make_pair(*solid, groups));
      opts->m_iTopoFaceCount += uFaceSize;
   }

   A3DRiRepresentationItemGet(NULL, &sRiData);
   A3DTess3DGet(NULL, &sTessData);
   A3DTessBaseGet(NULL, &sBaseTessData);

   return iRet;
}
/***A3DRiRepresentationItemCreatePTSolid******************************/

INTERNAL int traverseRepItem(const A3DRiRepresentationItem* pRepItem,
                             std::vector<void*> assemblyPath,
                             PTTransformMatrix transform,
                             A3DMiscCascadedAttributes* pFatherAttr,
                             A3DPolygonicaOptions& pgOpts,
                             A3D_log_func logging_function);

INTERNAL int traverseSet(const A3DRiSet* pSet,
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
         iRet = traverseRepItem(sData.m_ppRepItems[ui], assemblyPath, transform, pAttr, pgOpts, logging_function);
      }

      A3DRiSetGet(NULL, &sData);
   }

   return iRet;
}
/***traverseSet*******************************************************/

INTERNAL PTRenderStyle LookupRenderStyleByColor(float r, float g, float b,
                                                A3DPolygonicaOptions& pgOpts,
                                                A3D_log_func logging_function)
{
   // Maps color definitions to closest color in a palette 
   // to keep down number of styles used by Polygonica graphics
   auto nearest255 = [](float clrInt) { return (unsigned int)(clrInt * 255.0 + 0.5); };

   unsigned long lR = nearest255(r);
   unsigned long lG = nearest255(g);
   unsigned long lB = nearest255(b);
   unsigned long index = lR + (lG << 8) + (lB << 16);

   auto search = pgOpts.m_style_palette.find(index);
   if (search == pgOpts.m_style_palette.end())
   {
      float rgbcolor[] = { r,g,b };
      float grey[] = { 0.25f,0.25f, 0.25f };

      PTRenderStyle newStyle;
      PTPolygonStyle polyStyle;

      PFRenderStyleCreate(pgOpts.m_Environment, &newStyle);
      polyStyle = PFEntityGetEntityProperty(newStyle, PV_RSTYLE_PROP_POLYGON_STYLE);
      PFEntitySetColourProperty(polyStyle, PV_PSTYLE_PROP_COLOUR, PV_COLOUR_SINGLE_RGB_ARRAY, rgbcolor);
      PFEntitySetColourProperty(polyStyle, PV_PSTYLE_PROP_BACK_COLOUR, PV_COLOUR_SINGLE_RGB_ARRAY, rgbcolor);
      PFEntitySetNat32Property(polyStyle, PV_PSTYLE_PROP_TRANSPARENCY, 0);

      PTEdgeStyle edgeStyle;
      edgeStyle = PFEntityGetEntityProperty(newStyle, PV_RSTYLE_PROP_EDGE_STYLE);
      PFEntitySetColourProperty(edgeStyle, PV_ESTYLE_PROP_COLOUR, PV_COLOUR_SINGLE_RGB_ARRAY, grey);
      PFEntitySetEntityProperty(newStyle, PV_RSTYLE_PROP_EDGE_STYLE, 0);
      pgOpts.m_style_palette[index] = newStyle;
      return newStyle;
   }

   return search->second;
}
/***LookupRenderStyleByColor******************************************/

INTERNAL A3DStatus MultiplyMatrix(const double* padFather,
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
/***MultiplyMatrix****************************************************/

INTERNAL A3DVector3dData CrossProduct(const A3DVector3dData* X, const A3DVector3dData* Y)
{
   A3DVector3dData Z;
   Z.m_dX = X->m_dY * Y->m_dZ - X->m_dZ * Y->m_dY;
   Z.m_dY = X->m_dZ * Y->m_dX - X->m_dX * Y->m_dZ;
   Z.m_dZ = X->m_dX * Y->m_dY - X->m_dY * Y->m_dX;
   return Z;
}
/***CrossProduct****************************************************/

INTERNAL A3DStatus stTransform(A3DMiscTransformation* pTransformation,
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
      sZVector = CrossProduct(&(sTransformData.m_sXVector), &(sTransformData.m_sYVector));

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

      MultiplyMatrix((double*)transform, m, (double*)localTransform);

      A3DMiscCartesianTransformationGet(NULL, &sTransformData);
   }

   return iRet;
}
/***stTransform*****************************************************/

INTERNAL int traverseRepItem(const A3DRiRepresentationItem* pRepItem,
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
   CHECK_A3DSTATUS(stCreateAndPushCascadedAttributes(pRepItem, pFatherAttr, &pAttr, &sAttrData, logging_function),
      logging_function, "traverseRepItem - stCreateAndPushCascadedAttributes");
   const MiscCascadedAttributesGuard sMCAttrGuard(pAttr);

   float r, g, b;
   CHECK_A3DSTATUS(stExtractColorFromGraphicData(pRepItem, sAttrData.m_sStyle, r, g, b, logging_function),
      logging_function, "traverseRepItem - stExtractColorFromGraphicData");

   CHECK_A3DSTATUS(A3DEntityGetType(pRepItem, &eType),
      logging_function, "traverseRepItem - A3DEntityGetType");

   switch (eType)
   {
      case kA3DTypeRiSet:
      {
         iRet = traverseSet(pRepItem, assemblyPath, transform, pAttr, pgOpts, logging_function);
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

            iRet = stTransform(sCoordSysData.m_pTransformation, transform, localTransform, logging_function);

            A3DRiCoordinateSystemGet(NULL, &sCoordSysData);
         }

         A3DRiRepresentationItemGet(NULL, &sData);

         // Create a PTSolid and add a representation item / solid pair to the output map m_parts
         PTSolid solid;
         if (pgOpts.m_parts.find(pRepItem) == pgOpts.m_parts.end())
         {
            iRet = A3DRiRepresentationItemCreatePTSolid(pRepItem, &solid, &pgOpts);
            pgOpts.m_parts.insert(std::make_pair(pRepItem, solid));
         }
         else
         {
            solid = pgOpts.m_parts[pRepItem];
         }

         PTWorldEntity worldEntity;
         status = PFWorldAddEntity(pgOpts.m_World, solid, &worldEntity);
         CHECK_PTSTATUS(status, logging_function, "traverseRepItem - PFWorldAddEntity");
         if (status == PV_STATUS_OK)
         {
            status = PFWorldEntitySetTransform(worldEntity, localTransform, NULL);

            // Add the polygon render style to the output map m_style_palette if required
            PTRenderStyle poly_style = LookupRenderStyleByColor(r, g, b, pgOpts, logging_function);
            PFEntitySetEntityProperty(worldEntity, PV_WENTITY_PROP_STYLE, poly_style);

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
         log(logging_function, "traverseRepItem of type " + std::to_string(eType) + " is not implemented", A3D_LOG_WARN);
         break;
      }
   }
   return iRet;
}
/***traverseRepItem*************************************************/

INTERNAL int stTraversePartDef(const A3DAsmPartDefinition* pPart,
                               std::vector<void*> assemblyPath, 
                               PTTransformMatrix transform, 
                               A3DMiscCascadedAttributes* pFatherAttr, 
                               A3DPolygonicaOptions& pgOpts, 
                               A3D_log_func logging_function)
{
   A3DInt32 iRet = A3D_SUCCESS;

   A3DMiscCascadedAttributes* pAttr;
   A3DMiscCascadedAttributesData sAttrData;
   CHECK_A3DSTATUS(stCreateAndPushCascadedAttributes(pPart, pFatherAttr, &pAttr, &sAttrData, logging_function), 
                   logging_function, "stTraversePartDef - stCreateAndPushCascadedAttributes");
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
         traverseRepItem(sData.m_ppRepItems[ui], assemblyPath, transform, pAttr, pgOpts, logging_function);
      }

      A3DAsmPartDefinitionGet(NULL, &sData);
   }

   assemblyPath.pop_back();

   return iRet;
}
/***stTraversePartDef***********************************************/

INTERNAL int stTraversePOccurrence(const A3DAsmProductOccurrence* pOccurrence,
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
   CHECK_A3DSTATUS(stCreateAndPushCascadedAttributes(pOccurrence, pFatherAttr, &pAttr, &sAttrData, logging_function), 
                   logging_function, "stTraversePOccurrence - stCreateAndPushCascadedAttributes");
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
         if (eType == kA3DTypeMiscCartesianTransformation)
         {
            iRet = stTransform(sData.m_pLocation, transform, localTransform, logging_function);
         }
         else if (eType == kA3DTypeMiscGeneralTransformation)
         {
            log(logging_function, "In stTraversePOccurrence, cannot process location type kA3DTypeMiscGeneralTransformation", A3D_LOG_ERROR);
         }
         else
         {
            log(logging_function, "In stTraversePOccurrence, the location type " + std::to_string(eType) + " is unknown", A3D_LOG_ERROR);
         }
      }

      if (!isPrototype)
      {
         assemblyPath.push_back((void*)pOccurrence);
      }

      if (sData.m_pPrototype)
      {
         stTraversePOccurrence(sData.m_pPrototype, assemblyPath, localTransform, pAttr, true, pgOpts, logging_function);
      }
      else if (sData.m_pExternalData)
      {
         stTraversePOccurrence(sData.m_pExternalData, assemblyPath, localTransform, pAttr, true, pgOpts, logging_function);
      }
      else
      {
         for (ui = 0; ui < sData.m_uiPOccurrencesSize; ++ui)
         {
            stTraversePOccurrence(sData.m_ppPOccurrences[ui], assemblyPath, localTransform, pAttr, false, pgOpts, logging_function);
         }
      }

      if (sData.m_pPart)
      {
         stTraversePartDef(sData.m_pPart, assemblyPath, localTransform, pAttr, pgOpts, logging_function);
      }

      if (!isPrototype)
      {
         assemblyPath.pop_back();
      }
      CHECK_A3DSTATUS(A3DAsmProductOccurrenceGet(NULL, &sData), logging_function, "stTraversePOccurrence - A3DAsmProductOccurrenceGet");
   }

   return iRet;
}
/***stTraversePOccurrence*******************************************/

/*!
\brief Creates a Polygonica world and PTSolids list from the provided model.
\param pModelFile The model file to parse solids and transforms. Should contain A3DRiPolyBrep or A3DRiBrepModel
\param A3DPolygonicaOptions [out] opts The structure containing the resultant world populated with solids
\return A3D_SUCCESS - Operation succeeded
  A3D_PG_NOT_INITIALIZED - Polygonica was not unlocked or initialized correctly
  A3D_PG_INVALID_RI - Representation item is unsupported type
  A3D_PG_ERROR - Internal polygonica error
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
   CHECK_A3DSTATUS(A3DMiscCascadedAttributesCreate(&pAttr), logging_function, "A3DModelCreatePTWorld");
   const MiscCascadedAttributesGuard sMCAttrGuard(pAttr);

   iRet = A3DAsmModelFileGet(pModelFile, &sData);
   if (iRet == A3D_SUCCESS)
   {
      for (A3DUns32 ui = 0; ui < sData.m_uiPOccurrencesSize; ++ui)
      {
         stTraversePOccurrence(sData.m_ppPOccurrences[ui], assemblyPath, transform, pAttr, false, pgOpts, logging_function);
      }
      CHECK_A3DSTATUS(A3DAsmModelFileGet(NULL, &sData), logging_function, "A3DModelCreatePTWorld - A3DAsmModelFileGet");
   }

   return iRet;
}
/***A3DModelCreatePGWorld*******************************************/

INTERNAL int A3DDestroyBridgeSolids(A3DPolygonicaOptions& bridge_data)
{
   /* Destroy PTSolids created by the bridge */
   for (auto i = bridge_data.m_parts.begin();
        i != bridge_data.m_parts.end(); i++)
   {
      PFSolidDestroy((PTSolid)i->second);
   }
   return A3D_SUCCESS;
}
/***A3DDestroyBridgeSolids******************************************/

INTERNAL int A3DDestroyBridgeWorldEntities(A3DPolygonicaOptions& bridge_data)
{
   /* Destroy PTWorldEntities created by the bridge */
   for (auto i = bridge_data.m_entities.begin();
        i != bridge_data.m_entities.end(); i++)
   {
      PFWorldRemoveEntity((PTWorldEntity)*i);
   }
   return A3D_SUCCESS;
}
/***A3DDestroyBridgeWorldEntities***********************************/

INTERNAL int A3DDestroyBridgePartsData(A3DPolygonicaOptions& bridge_data)
{
   /* Destroy parts data created by the bridge in the A3DPolygonicaOptions struct */
   /* NB This does not destroy the PTSolids */
   // Currently not required as the unordered_map will be cleaned up when it goes out of scope
   // This function left in as it may be required if m_parts is replaced with a non-C++ type
   bridge_data.m_parts.clear();
   return A3D_SUCCESS;
}
/***A3DDestroyBridgePartsData***************************************/

INTERNAL int A3DDestroyBridgeEntitiesData(A3DPolygonicaOptions& bridge_data)
{
   /* Destroy vector of entities created by the bridge in the A3DPolygonicaOptions struct */
   /* NB This does not destroy the PTWorldEntities */
   // Currently not required as the vector will be cleaned up when it goes out of scope
   // This function left in as it may be required if m_entities is replaced with a non-C++ type
   bridge_data.m_entities.clear();
   return A3D_SUCCESS;
}
/***A3DDestroyBridgeEntitiesData************************************/

INTERNAL int A3DDestroyBridgeStylesData(A3DPolygonicaOptions& bridge_data)
{
   /* Destroy render styles data created by the bridge in the A3DPolygonicaOptions struct */
   for (auto i = bridge_data.m_style_palette.begin();
        i != bridge_data.m_style_palette.end(); i++)
   {
      PFRenderStyleDestroy((PTRenderStyle)i->second);
   }
   // Currently not required as the map will be cleaned up when it goes out of scope
   // This function left in as it may be required if m_style_palette is replaced with a non-C++ type
   bridge_data.m_style_palette.clear();
   return A3D_SUCCESS;
}
/***A3DDestroyBridgeStylesData**************************************/

INTERNAL int A3DDestroyBridgeSurfaceGroupsData(A3DPolygonicaOptions& bridge_data)
{
   /* Destroy surface groups data created by the bridge in the A3DPolygonicaOptions struct */
   for (auto i = bridge_data.m_surface_groups.begin();
        i != bridge_data.m_surface_groups.end(); i++)
   {
      std::vector <PTEntityGroup>* groups = (std::vector <PTEntityGroup>*)i->second;
      for (int topoFace = 0; topoFace < groups->size(); topoFace++)
      {
         PFEntityGroupDestroy((*groups)[topoFace]);
      }
      delete groups;
   }
   // Currently not required as the unordered_map will be cleaned up when it goes out of scope
   // This function left in as it may be required if m_surface_groups is replaced with a non-C++ type
   bridge_data.m_surface_groups.clear();
   return A3D_SUCCESS;
}
/***A3DDestroyBridgeSurfaceGroupsData*******************************/

INTERNAL int A3DDestroyBridgePathsData(A3DPolygonicaOptions& bridge_data)
{
   /* Destroy path data created by the bridge in the A3DPolygonicaOptions struct */
   for (auto i = bridge_data.m_paths.begin(); 
        i != bridge_data.m_paths.end(); i++)
   {
      std::vector<void*>* path = (std::vector<void*>*)i->second;
      delete path;
   }
   // Currently not required as the unordered_map will be cleaned up when it goes out of scope
   // This function left in as it may be required if m_paths is replaced with a non-C++ type
   bridge_data.m_paths.clear();
   return A3D_SUCCESS;
}
/***A3DDestroyBridgePathsData***************************************/

INTERNAL int A3DDestroyBridgeData(A3DPolygonicaOptions& bridge_data)
{
   /* Destroy data created by the bridge in the A3DPolygonicaOptions struct */
   /* NB This does not destroy the PTSolids or PTWorldEntities */
   /* Use A3DDestroyBridgeSolids and A3DDestroyBridgeWorldEntities if this is required */

   A3DDestroyBridgePartsData(bridge_data);
   A3DDestroyBridgeEntitiesData(bridge_data);
   A3DDestroyBridgeStylesData(bridge_data);
   A3DDestroyBridgeSurfaceGroupsData(bridge_data);
   A3DDestroyBridgePathsData(bridge_data);

   return A3D_SUCCESS;
}
/***A3DDestroyBridgeData********************************************/
