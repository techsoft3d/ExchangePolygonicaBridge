// TranslateToPgSolids.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define INITIALIZE_A3D_API
#include <A3DSDKIncludes.h>

#include "common.hpp"
#include <../../../../include/ExchangePolygonicaBridge.h>

#include "pg/pgapi.h"
#include "pg/pgrender.h"
#include "pg/pgmacros.h"
#include "window_wide_strings.h"

A3DPolygonicaOptions pgOpts;

static MY_CHAR acSrcFileName[_MAX_PATH * 2];

static void handle_pg_error(PTStatus status, char* err_string)
{
	PTStatus status_code;
	PTStatus err_code;
	PTStatus func_code;
	PTStatus fatal_error = PV_STATUS_OK;
	/* The status is made up of 3 parts */
	status_code = PM_STATUS_FROM_API_ERROR_CODE(status);
	func_code = PM_FN_FROM_API_ERROR_CODE(status);
	err_code = PM_ERR_FROM_API_ERROR_CODE(status);
	if (status_code & PV_STATUS_BAD_CALL)
	{
		printf("PG:BAD_CALL: Function % d Error % d: % s\n", func_code, err_code, err_string);
	}
	if (status_code & PV_STATUS_MEMORY)
	{
		printf("PG:MEMORY: Function % d Error % d: % s\n", func_code, err_code, err_string);
			fatal_error |= status;
	}
	if (status_code & PV_STATUS_EXCEPTION)
	{
		printf("PG:EXCEPTION: Function % d Error % d: % s\n", func_code, err_code, err_string);
			fatal_error |= status;
	}
	if (status_code & PV_STATUS_FILE_IO)
	{
		printf("PG:FILE I/0: Function % d Error % d: % s\n", func_code, err_code, err_string);
	}
	if (status_code & PV_STATUS_INTERRUPT)
	{
		printf("PG:INTERRUPT: Function %d Error %d: %s\n",
			func_code, err_code, err_string);
	}
	if (status_code & PV_STATUS_INTERNAL_ERROR)
	{
		printf("PG:INTERNAL_ERROR: Function %d Error %d: %s\n",
			func_code, err_code, err_string);
		fatal_error |= status;
	}
}

static PTEntityGroup findRegionFromFaceAppData(PTFace face)
{
	PTSolid solid = PFEntityGetEntityProperty(face, PV_FACE_PROP_SOLID);
	PTEnvironment env = PFEntityGetEntityProperty(solid, PV_SOLID_PROP_ENVIRONMENT);
	PTEntityGroup region = PV_ENTITY_NULL;
	void* topoFace = PFEntityGetPointerProperty(face, PV_FACE_PROP_APP_DATA);
	// Use a category to find all the faces with the same topoface, and highlight them

	PTCategoryOpts options;
	PMInitCategoryOpts(&options);
	options.fn_face = face_in_category_cb;
	options.app_data = (PTPointer)topoFace;

	PTCategory category;
	PFCategoryCreate(env, PV_CRITERION_CALLBACK, &options, &category);

	PFEntityGroupCreate(env, &region);
	PFEntityGroupCreateFromCategory(solid, category, &region);
	PFCategoryDestroy(category);

	return region;
}

int wmain(A3DInt32 iArgc, A3DUniChar** ppcArgv)
{
    //
    // ### COMMAND LINE ARGUMENTS
    //

    if (iArgc < 2)
    {
        MY_PRINTF2("Usage:\n %s <input CAD file>\n", ppcArgv[0]);
        return A3D_ERROR;
    }
    
    MY_STRCPY(acSrcFileName, ppcArgv[1]);

    //
    // ### INITIALIZE HOOPS EXCHANGE
    //

    A3DSDKHOOPSExchangeLoader sHoopsExchangeLoader(_T(HOOPS_BINARY_DIRECTORY));
    CHECK_RET(sHoopsExchangeLoader.m_eSDKStatus);

	//
	// ### INITIALIZE POLYGONICA
	//

	PTStatus status;

	PTInitialiseOpts initialise_options;
	PMInitInitialiseOpts(&initialise_options);
	status = PFInitialise(PV_LICENSE, &initialise_options);
	if (status != PV_STATUS_OK) return status;
	
	PTEnvironmentOpts env_options;
	PMInitEnvironmentOpts(&env_options);
	
	status = PFEnvironmentCreate(&env_options, &pgOpts.m_Environment);

	if (status != PV_STATUS_OK) return status;

	PFEntitySetPointerProperty(pgOpts.m_Environment, PV_ENV_PROP_ERROR_REPORT_CB, handle_pg_error);

	/* Create a window to render into */
	HWND window;
	window = PgWindowCreate(L"TranslateToPgSolids", 100, 100, 1200, 900);

	PTDrawable drawable;
	status = PFDrawableCreate(pgOpts.m_Environment, window, NULL, &drawable);

	status = PFWorldCreate(pgOpts.m_Environment, NULL, &pgOpts.m_World);

    //
    // ### PROCESS SAMPLE CODE
    //

	A3DRWParamsLoadData load_params;
	A3D_INITIALIZE_DATA(A3DRWParamsLoadData, load_params);
	load_params.m_sGeneral.m_eReadGeomTessMode = kA3DReadTessOnly;

	A3DImport sImport(acSrcFileName);
	sImport.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = A3DEReadGeomTessMode::kA3DReadTessOnly;

	const A3DStatus iRet = sHoopsExchangeLoader.Import(sImport);
	CHECK_RET(iRet);

	A3DModelCreatePGWorld(sHoopsExchangeLoader.m_psModelFile, pgOpts);

	PTViewport vp;
	PTPoint vp_from = { 1.0, 0.8, 0.6 };
	PTPoint vp_to = { 0.0, 0.0, 0.0 };
	PTVector vp_up = { 0.0, 0.0, 1.0 };
	status = PFViewportCreate(pgOpts.m_World, &vp);
	status = PFViewportSetPinhole(vp, vp_from, vp_to, vp_up, PV_PROJ_PERSPECTIVE, 50.0);
	PTBounds bounds;
	PFEntityGetBoundsProperty(pgOpts.m_World, PV_WORLD_PROP_BOUNDS, bounds);
	status = PFViewportFit(vp, bounds);
	status = PgWindowRegister(window, drawable, vp);

	// Print entitie paths
	for (int i = 0; i < pgOpts.m_entities.size(); i++)
	{
		std::vector<void*> *path = (std::vector<void*> * )PFEntityGetPointerProperty(pgOpts.m_entities[i], PV_WORLD_PROP_APP_DATA);
		for (int j = 0; j < path->size(); j++)
		{
			A3DEEntityType eType = kA3DTypeUnknown;
			void* pNode = path->at(j);
			A3DEntityGetType(pNode, &eType);
			if (eType == kA3DTypeAsmProductOccurrence)
			{
				std::string name;
				stGetName(pNode, name);
				printf("%s | ", name.c_str());
			}
		}
		printf("\n");
	}

	do
	{
		PTNat32 mouse_x, mouse_y;
		PgWindowMouse(L"Double click to select a face", &mouse_x, &mouse_y);

		PTPick pick;
		status = PFPickCreateFromScreen(drawable, vp, mouse_x, mouse_y, &pick);
		PTPickDirectResult result;
		status = PFPickDirect(pick, PV_PICK_TYPE_SOLID, NULL, &result);

		PTEntityGroup entityGroup;
		status = PFPickEntityGroup(pick, PV_PICK_TARGET_FACE, NULL, &entityGroup);

		// Highlight a face
		PTHighlight highlight;

		/* Setup the polygon style (once set it may be destroyed) */
		double blue[3] = { 0.0, 0.0, 0.8 };
		PTRenderStyle render_style;
		PTPolygonStyle poly_style;
		PFPolygonStyleCreate(pgOpts.m_Environment, &poly_style);
		PFEntitySetColourProperty(poly_style, PV_PSTYLE_PROP_COLOUR,
			PV_COLOUR_DOUBLE_RGB_ARRAY, blue);
		PFEntitySetNat32Property(poly_style, PV_PSTYLE_PROP_TRANSPARENCY, 0);
		PFEntitySetBooleanProperty(poly_style, PV_PSTYLE_PROP_2_SIDES, TRUE);

		/* Setup a render style for the highlighted faces */
		PFRenderStyleCreate(pgOpts.m_Environment, &render_style);

		PFEntitySetEntityProperty(render_style, PV_RSTYLE_PROP_POLYGON_STYLE, poly_style);
		PFPolygonStyleDestroy(poly_style);

		if (entityGroup)
		{
			PTEntityList list;
			PFEntityCreateEntityList(entityGroup, PV_ENTITY_TYPE_FACE, NULL, &list);
			PTFace face = PFEntityListGetFirst(list);
			PFEntityListDestroy(list, 0);

			if (face)
			{
				PTSolid solid = PFEntityGetEntityProperty(face, PV_FACE_PROP_SOLID);
				PTEntityGroup selectedRegion = findRegionFromFaceAppData(face);
				status = PFHighlightCreate(solid, selectedRegion, &highlight);
				//PFEntityGroupDestroy(selectedRegion);
			}

			PFEntityGroupDestroy(entityGroup);
		}

		PFEntitySetEntityProperty(highlight, PV_HLIGHT_PROP_STYLE, render_style);
		PFRenderStyleDestroy(render_style);
	} while (1);
	 
	//
	// ### CLEAN UP
	//
	PgWindowText(L"Press any key to terminate");

	destroyBridge(pgOpts);

	status = PFWorldDestroy(pgOpts.m_World);

	if (pgOpts.m_Environment)
		PFEnvironmentDestroy(pgOpts.m_Environment);

	PFTerminate();

    return 0;
}