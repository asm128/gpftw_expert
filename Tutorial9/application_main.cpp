// Tip: Hold Left ALT + SHIFT while tapping or holding the arrow keys in order to select multiple columns and write on them at once. 
//		Also useful for copy & paste operations in which you need to copy a bunch of variable or function names and you can't afford the time of copying them one by one.
#include "application.h"

#include "llc_bitmap_target.h"
#include "llc_bitmap_file.h"
#include "llc_gui_text.h"

#include "llc_app_impl.h"
#include "llc_ro_rsm.h"
#include "llc_ro_rsw.h"

#include <process.h>

static constexpr	const uint32_t										ASCII_SCREEN_WIDTH							= 132	;
static constexpr	const uint32_t										ASCII_SCREEN_HEIGHT							= 50	;

LLC_DEFINE_APPLICATION_ENTRY_POINT(::SApplication);	

					::SApplication										* g_ApplicationInstance						= 0;

static				::llc::error_t										updateSizeDependentResources				(::SApplication& applicationInstance)											{ 
	const ::llc::SCoord2<uint32_t>												newSize										= applicationInstance.Framework.MainDisplay.Size;// / 2; 
	::llc::STexture<::llc::SColorBGRA>											& offscreen									= applicationInstance.Framework.Offscreen;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	if(newSize != offscreenMetrics) {
		::llc::SMatrix4<float>														& finalProjection							= applicationInstance.Scene.Transforms.FinalProjection	;
		::llc::SMatrix4<float>														& fieldOfView								= applicationInstance.Scene.Transforms.FieldOfView		;
		::llc::SMatrix4<float>														& viewport									= applicationInstance.Scene.Transforms.Viewport			;
		::llc::SMatrix4<float>														& viewportInverse							= applicationInstance.Scene.Transforms.ViewportInverse	;
		::llc::SMatrix4<float>														& viewportInverseCentered					= applicationInstance.Scene.Transforms.ViewportInverse	;
		::llc::SCameraRange															& cameraRange								= applicationInstance.Scene.Camera.Range;
		fieldOfView.FieldOfView(cameraRange.Angle * ::llc::math_pi, newSize.x / (double)newSize.y, cameraRange.Near, cameraRange.Far);
		viewport.Viewport(newSize, cameraRange.Far, cameraRange.Near);
		viewportInverse															= viewport.GetInverse();
		const ::llc::SCoord2<int32_t>												screenCenter								= {(int32_t)newSize.x / 2, (int32_t)newSize.y / 2};
		viewportInverseCentered													= viewportInverse;
		viewportInverseCentered._41												+= screenCenter.x;
		viewportInverseCentered._42												+= screenCenter.y;
		finalProjection															= fieldOfView * viewportInverseCentered;
		applicationInstance.Scene.Transforms.FinalProjectionInverse				= finalProjection.GetInverse();
	}
	llc_necall(::llc::updateSizeDependentTarget(offscreen											, newSize), "??");
	llc_necall(::llc::updateSizeDependentTarget(applicationInstance.Framework.OffscreenDepthStencil	, newSize), "??");
	return 0;
}
	
// --- Cleanup application resources.
static				::llc::error_t										cleanup										(::SApplication& applicationInstance)											{
	::llc::SDisplayPlatformDetail												& displayDetail								= applicationInstance.Framework.MainDisplay.PlatformDetail;
	if(displayDetail.WindowHandle) {
		error_if(0 == ::DestroyWindow(displayDetail.WindowHandle), "Not sure why would this fail.");
		error_if(errored(::llc::displayUpdate(applicationInstance.Framework.MainDisplay)), "Not sure why this would fail");
	}
	::UnregisterClass(displayDetail.WindowClassName, displayDetail.WindowClass.hInstance);

	g_ApplicationInstance													= 0;
	return 0;
}

static				::llc::error_t										bmpOrBmgLoad								(::llc::view_string bmpFileName, ::llc::STexture<::llc::SColorBGRA>& loaded)		{
	::llc::view_const_string													bmpFileNameC								= {bmpFileName.begin(), bmpFileName.size()};
	if(errored(::llc::bmpFileLoad(bmpFileNameC, loaded))) {
		error_printf("Failed to load bitmap from file: %s.", bmpFileNameC.begin());
		bmpFileName[bmpFileName.size() - 2]										= 'g';
		llc_necall(::llc::bmgFileLoad(bmpFileNameC, loaded), "Failed to load bitmap from file: %s.", bmpFileNameC.begin());
	}
	::llc::STexture<::llc::SColorBGRA>											original									= loaded;
	for(uint32_t y = 0; y < original.View.metrics().y; ++y)
		memcpy(loaded[y].begin(), original[original.View.metrics().y - 1 - y].begin(), sizeof(::llc::SColorBGRA) * original.View.metrics().x);
	return 0;
}

static				::llc::error_t										setNodeHierarchyIds							(const ::llc::array_obj<::llc::SRSMNode>& rsmNodes, const uint32_t iNode, ::llc::SModelHierarchyNodeRSM& modelHierarchyNode)	{
	const ::llc::SRSMNode														& node										= rsmNodes[iNode];
	modelHierarchyNode.IdParent												= -1;
	modelHierarchyNode.IdChildren.clear();
	for(uint32_t iOtherNode = 0; iOtherNode  < rsmNodes.size(); ++iOtherNode) {
		const ::llc::SRSMNode														& nodeOther									= rsmNodes[iOtherNode];
			 if(0 == strcmp(nodeOther.ParentName, node.Name))	modelHierarchyNode.IdChildren.push_back(iOtherNode);
		else if(0 == strcmp(nodeOther.Name, node.ParentName))	modelHierarchyNode.IdParent				= iOtherNode;

	}
	return 0;
}

static				::llc::error_t										getGlobalTransform							(const ::llc::array_view<::llc::SModelHierarchyNodeRSM>& rsmNodes, uint32_t iNode, const ::llc::array_view<::llc::SMatrix4<float>>& localTransforms, ::llc::array_view<::llc::SMatrix4<float>>& globalTransforms)					{
	uint32_t																	indexParent									= (uint32_t)rsmNodes[iNode].IdParent;
	if(indexParent < rsmNodes.size()) {
		getGlobalTransform(rsmNodes, indexParent, localTransforms, globalTransforms);
		globalTransforms[iNode]													= localTransforms[iNode] * globalTransforms[indexParent];
	}
	else
		globalTransforms[iNode]													= localTransforms[iNode];
	return 0;
}

					::llc::error_t										mainWindowCreate							(::llc::SDisplay& mainWindow, HINSTANCE hInstance);
static				::llc::error_t										setup										(::SApplication& applicationInstance)													{
	g_ApplicationInstance													= &applicationInstance;
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	error_if(errored(::mainWindowCreate(mainWindow, applicationInstance.Framework.RuntimeValues.PlatformDetail.EntryPointArgs.hInstance)), "Failed to create main window why?????!?!?!?!?");
	char																		bmpFileName2	[]							= "..\\gpk_data\\images\\Codepage-437-24.bmp";
	error_if(errored(::bmpOrBmgLoad(bmpFileName2, applicationInstance.TextureFont)), "");
	const ::llc::SCoord2<uint32_t>												& textureFontMetrics						= applicationInstance.TextureFont.View.metrics();
	applicationInstance.TextureFontMonochrome.resize(textureFontMetrics);
	for(uint32_t y = 0, yMax = textureFontMetrics.y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = textureFontMetrics.x; x < xMax; ++x) {
		const ::llc::SColorBGRA														& srcColor									= applicationInstance.TextureFont.View[y][x];
		applicationInstance.TextureFontMonochrome.View[y * textureFontMetrics.x + x]	
			=	0 != srcColor.r
			||	0 != srcColor.g
			||	0 != srcColor.b
			;
	}
	static constexpr const char													rsmFilename	[]								= "니플헤임\\니플헤임-풍차.rsm";
	static constexpr const char													ragnaPath	[]								= "..\\data_2005\\data\\";
	char																		temp		[512]							= {};
	::llc::SRSMFileContents														& rsmData									= applicationInstance.RSMData;
	sprintf_s(temp, "%s%s%s", ragnaPath, "model\\", rsmFilename);	
	llc_necall(::llc::rsmFileLoad(rsmData, ::llc::view_const_string(temp)), "Error");
	llc_necall(applicationInstance.RSMTextures.resize(rsmData.TextureNames.size()), "Why would this fail? Corrupt RSM?");
	for(uint32_t iTex = 0, texCount = rsmData.TextureNames.size(); iTex < texCount; ++iTex) {
		sprintf_s(temp, "%s%s%s", ragnaPath, "texture\\", &rsmData.TextureNames[iTex][0]);	
		ce_if(errored(::llc::bmpFileLoad(::llc::view_const_string(temp), applicationInstance.RSMTextures[iTex])), "File not found? %s.", &rsmData.TextureNames[iTex][0]);
		info_printf("RSM BMP loaded successfully: %s.", &rsmData.TextureNames[iTex][0]);
	}
	const uint32_t																countNodes									= rsmData.Nodes.size();
	applicationInstance.RSMTransformGlobal	.resize(countNodes);
	applicationInstance.RSMTransformLocal	.resize(countNodes);
	applicationInstance.RSMHierarchy		.resize(countNodes);
	for(uint32_t iNode = 0; iNode < countNodes; ++iNode)  {
		rsmData.Nodes[iNode].Transform.RotAxis.Normalize();
		setNodeHierarchyIds(rsmData.Nodes, iNode, applicationInstance.RSMHierarchy[iNode]);
	}
	::llc::SMatrix4<float>														nodeLocalScale;
	::llc::SMatrix4<float>														nodeLocalTranslation;
	::llc::SMatrix4<float>														nodeLocalRotation;
	::llc::SMatrix4<float>														nodeLocalOffset;
	for(uint32_t iNode = 0; iNode < countNodes; ++iNode) {
		applicationInstance.RSMTransformLocal	[iNode].Identity();
		applicationInstance.RSMTransformGlobal	[iNode].Identity();
		
		const ::llc::SRSMNode														& rsmNode									= rsmData.Nodes[iNode];
		//if(iNode == 0) {
			applicationInstance.RSMTransformLocal	[iNode]							= 
				{ rsmNode.Transform.Row0.x, rsmNode.Transform.Row0.y, rsmNode.Transform.Row0.z, 0
				, rsmNode.Transform.Row1.x, rsmNode.Transform.Row1.y, rsmNode.Transform.Row1.z, 0
				, rsmNode.Transform.Row2.x, rsmNode.Transform.Row2.y, rsmNode.Transform.Row2.z, 0
				, 0, 0, 0, 1
				};
		//	}
			//applicationInstance.RSMTransformLocal	[iNode]							= 
			//	{ rsmNode.Transform.Row0.x, rsmNode.Transform.Row1.x, rsmNode.Transform.Row2.x, 0
			//	, rsmNode.Transform.Row0.y, rsmNode.Transform.Row1.y, rsmNode.Transform.Row2.y, 0
			//	, rsmNode.Transform.Row0.z, rsmNode.Transform.Row1.z, rsmNode.Transform.Row2.z, 0
			//	, 0, 0, 0, 1
			//	};
		
		nodeLocalOffset			.Identity();
		nodeLocalScale			.Identity();
		nodeLocalRotation		.Identity();
		nodeLocalTranslation	.Identity();

		const double																degrees										= rsmNode.Transform.Rotation; 
		//const double																degrees										= rsmNode.Transform.Rotation * (180 / (float)::llc::math_pi);
		nodeLocalOffset			.SetTranslation				({rsmNode.Transform.Offset		.x, rsmNode.Transform.Offset		.z, rsmNode.Transform.Offset		.y}, true);
		nodeLocalScale			.Scale						({rsmNode.Transform.Scale		.x, rsmNode.Transform.Scale			.z, rsmNode.Transform.Scale			.y}, true);
		if(rsmNode.RotationKeyframes.size()) {
			nodeLocalRotation.Identity();
			nodeLocalRotation.SetOrientation(rsmNode.RotationKeyframes[0].Orientation);
		}
		else
			nodeLocalRotation		.RotationArbitraryAxis		({rsmNode.Transform.RotAxis		.x, rsmNode.Transform.RotAxis		.z, rsmNode.Transform.RotAxis		.y}, (float)degrees);
		//nodeLocalRotation		.rotate						({rsmNode.Transform.RotAxis		.x, rsmNode.Transform.RotAxis		.z, rsmNode.Transform.RotAxis		.y}, (float)degrees);
                                                               
		nodeLocalTranslation	.SetTranslation				({rsmNode.Transform.Translation	.x, rsmNode.Transform.Translation	.z, rsmNode.Transform.Translation	.y}, true);
		double l = rsmNode.Transform.RotAxis.Length(); l;

		applicationInstance.RSMTransformLocal	[iNode]							*= nodeLocalOffset		;
		applicationInstance.RSMTransformLocal	[iNode]							*= nodeLocalScale		;
		applicationInstance.RSMTransformLocal	[iNode]							*= nodeLocalRotation	;
		applicationInstance.RSMTransformLocal	[iNode]							*= nodeLocalTranslation	;

	}
	for(uint32_t iNode = 0; iNode < countNodes; ++iNode) 
		getGlobalTransform(applicationInstance.RSMHierarchy, iNode, applicationInstance.RSMTransformLocal, applicationInstance.RSMTransformGlobal);

	llc_necall(applicationInstance.RSMNodes.resize(rsmData.Nodes.size() * applicationInstance.RSMData.TextureNames.size()), "Why would this fail? Corrupt RSM?");
	::llc::rsmGeometryGenerate(rsmData, applicationInstance.RSMNodes);

	ree_if(errored(::updateSizeDependentResources(applicationInstance)), "Cannot update offscreen and textures and this could cause an invalid memory access later on.");
	applicationInstance.Scene.Camera.Points.Position						= {0, 30, -20};
	applicationInstance.Scene.Camera.Points.Position						*= 10.0;
	applicationInstance.Scene.Camera.Range.Far								= 1000;
	applicationInstance.Scene.Camera.Range.Near								= 0.001;
	return 0;
}

static				::llc::error_t										update										(::SApplication& applicationInstance, bool systemRequestedExit)					{ 
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFrameInfo															& frameInfo									= framework.FrameInfo;
	retval_info_if(1, systemRequestedExit, "Exiting because the runtime asked for close. We could also ignore this value and just continue execution if we don't want to exit.");
	::llc::error_t																frameworkResult								= ::llc::updateFramework(applicationInstance.Framework);
	ree_if(errored(frameworkResult), "Unknown error.");
	rvi_if(1, frameworkResult == 1, "Framework requested close. Terminating execution.");
	ree_if(errored(::updateSizeDependentResources(applicationInstance)), "Cannot update offscreen and this could cause an invalid memory access later on.");

	//----------------------------------------------
	bool																		updateProjection							= false;
	if(framework.Input.KeyboardCurrent.KeyState[VK_ADD		])	{ updateProjection = true; applicationInstance.Scene.Camera.Range.Angle += frameInfo.Seconds.LastFrame * .05f; }
	if(framework.Input.KeyboardCurrent.KeyState[VK_SUBTRACT	])	{ updateProjection = true; applicationInstance.Scene.Camera.Range.Angle -= frameInfo.Seconds.LastFrame * .05f; }
	if(updateProjection) {
		::llc::STexture<::llc::SColorBGRA>											& offscreen									= framework.Offscreen;
		const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();

		::llc::SMatrix4<float>														& finalProjection							= applicationInstance.Scene.Transforms.FinalProjection	;
		::llc::SMatrix4<float>														& fieldOfView								= applicationInstance.Scene.Transforms.FieldOfView		;
		::llc::SMatrix4<float>														& viewport									= applicationInstance.Scene.Transforms.Viewport			;
		::llc::SMatrix4<float>														& viewportInverse							= applicationInstance.Scene.Transforms.ViewportInverse	;
		::llc::SMatrix4<float>														& viewportInverseCentered					= applicationInstance.Scene.Transforms.ViewportInverse	;
		fieldOfView.FieldOfView(applicationInstance.Scene.Camera.Range.Angle * ::llc::math_pi, offscreenMetrics.x / (double)offscreenMetrics.y, applicationInstance.Scene.Camera.Range.Near, applicationInstance.Scene.Camera.Range.Far);
		viewport.Viewport(offscreenMetrics, applicationInstance.Scene.Camera.Range.Far, applicationInstance.Scene.Camera.Range.Near);
		viewportInverse															= viewport.GetInverse();
		const ::llc::SCoord2<int32_t>												screenCenter								= {(int32_t)offscreenMetrics.x / 2, (int32_t)offscreenMetrics.y / 2};
		viewportInverseCentered													= viewportInverse;
		viewportInverseCentered._41												+= screenCenter.x;
		viewportInverseCentered._42												+= screenCenter.y;
		finalProjection															= fieldOfView * viewportInverseCentered;
		applicationInstance.Scene.Transforms.FinalProjectionInverse				= finalProjection.GetInverse();
	}

	//------------------------------------------------ Camera
	::llc::SCameraPoints														& camera									= applicationInstance.Scene.Camera.Points;
	camera.Position.RotateY(framework.Input.MouseCurrent.Deltas.x / 20.0f / (applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_CONTROL] ? 2.0 : 1));
	if(framework.Input.MouseCurrent.Deltas.z) {
		::llc::SCoord3<float>														zoomVector									= camera.Position;
		zoomVector.Normalize();
		const double																zoomWeight									= framework.Input.MouseCurrent.Deltas.z * (applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_SHIFT] ? 10 : 1) / 240.;
		camera.Position															+= zoomVector * zoomWeight * .5;
	}
	::llc::SMatrix4<float>														& viewMatrix								= applicationInstance.Scene.Transforms.View;
	::llc::SCameraVectors														& cameraVectors								= applicationInstance.Scene.Camera.Vectors;
	cameraVectors.Up														= {0, 1, 0};
	cameraVectors.Front														= (camera.Target - camera.Position).Normalize();
	cameraVectors.Right														= cameraVectors.Up		.Cross(cameraVectors.Front).Normalize();
	cameraVectors.Up														= cameraVectors.Front	.Cross(cameraVectors.Right).Normalize();
	viewMatrix.View3D(camera.Position, cameraVectors.Right, cameraVectors.Up, cameraVectors.Front);
	//for(uint32_t iNode = 0; iNode < applicationInstance.RSMHierarchy.size(); ++iNode)
	//	if(applicationInstance.RSMHierarchy.size() <= (uint32_t)applicationInstance.RSMHierarchy[iNode].IdParent) {
	//		camera.Target	= applicationInstance.RSMData.Nodes[iNode].Transform.Translation;
	//		break;
	//	}
	//viewMatrix.LookAt(camera.Position, camera.Target, cameraVectors.Up);

	//------------------------------------------------ Lights
	::llc::SCoord3<float>														& lightDir									= applicationInstance.LightDirection;
	lightDir.RotateY(frameInfo.Microseconds.LastFrame / 1000000.0f);
	lightDir.Normalize();

	//------------------------------------------------ 
	//applicationInstance.GridPivot.Scale										= {2.f, 4.f, 2.f};
	applicationInstance.RSMPivot.Scale										= {1, -1, -1};
	applicationInstance.RSMPivot.Scale										/= 10.0;
	//applicationInstance.GridPivot.Orientation.y								= (float)(sinf((float)(frameInfo.Seconds.Total / 2)) * ::llc::math_2pi);
	//applicationInstance.GridPivot.Orientation.w								= 1;
	//applicationInstance.GridPivot.Orientation.Normalize();
	//applicationInstance.GridPivot.Position									= {applicationInstance.GNDData.Metrics.Size.x / 2.0f * -1, 0, applicationInstance.GNDData.Metrics.Size.y / 2.0f * -1};
	return 0;
}

					::llc::error_t										drawRSM										(::SApplication& applicationInstance);

static				::llc::error_t										draw										(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	::llc::STexture<uint32_t>													& offscreenDepth							= framework.OffscreenDepthStencil;
	::memset(offscreenDepth	.Texels.begin(), -1, sizeof(uint32_t)								* offscreenDepth	.Texels.size());	// Clear target.
	::memset(offscreen		.Texels.begin(),  0, sizeof(::llc::SFramework::TOffscreen::TTexel)	* offscreen			.Texels.size());	// Clear target.
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	for(uint32_t y = 0, yMax = offscreenMetrics.y; y < yMax; ++y) {	// Draw background gradient.
		const uint8_t																colorHeight									= (uint8_t)(y / 10);
		for(uint32_t x = 0, xMax = offscreenMetrics.x; x < xMax; ++x)
			offscreen.View[y][x]													= {colorHeight, 0, 0, 0xFF};
	}

	int32_t 																	pixelsDrawn2								= drawRSM(applicationInstance); error_if(errored(pixelsDrawn2), "??");
	static constexpr const ::llc::SCoord2<int32_t>								sizeCharCell								= {9, 16};
	uint32_t																	lineOffset									= 0;
	static constexpr const char													textLine2	[]								= "Press ESC to exit.";
	::llc::grid_view<::llc::SColorBGRA>											& offscreenView								= offscreen.View;
	const ::llc::grid_view<::llc::SColorBGRA>									& fontAtlasView								= applicationInstance.TextureFont.View;
	const ::llc::SColorBGRA														textColor									= {0, framework.FrameInfo.FrameNumber % 0xFFU, 0, 0xFFU};
	::llc::textLineDrawAlignedFixedSizeLit(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), lineOffset = offscreenMetrics.y / sizeCharCell.y - 1, offscreenMetrics, sizeCharCell, textLine2, textColor);
	::llc::STimer																& timer										= applicationInstance.Framework.Timer;
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	char																		buffer		[256]							= {};

	const ::SRenderCache														& renderCache								= applicationInstance.RenderCache;

	uint32_t
	textLen																	= (uint32_t)sprintf_s(buffer, "[%u x %u]. FPS: %g. Last frame seconds: %g."					, mainWindow.Size.x, mainWindow.Size.y, 1 / timer.LastTimeSeconds, timer.LastTimeSeconds);							::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangles drawn: %u. Pixels drawn: %u. Pixels skipped: %u."	, (uint32_t)renderCache.TrianglesDrawn, (uint32_t)renderCache.PixelsDrawn, (uint32_t)renderCache.PixelsSkipped);	::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//::textDrawAlignedFixedSize(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen}, textColor);
	return 0;																																																
}
