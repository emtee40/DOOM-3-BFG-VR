#define NOMINMAX
#include "gl/scene/gl_rift_hmd.h"
#include "gl/system/gl_system.h"
#include "gl/system/gl_cvars.h"
#include <cstring>
#include <string>
#include <sstream>

extern "C" {
// #include "OVR.h"
#include "OVR_CAPI_GL.h"
}

#include "Extras/OVR_Math.h"

using namespace std;

RiftHmd::RiftHmd()
	: hmd(nullptr)
	, sceneFrameBuffer(0)
	, depthBuffer(0)
	, sceneTextureSet(nullptr)
	, mirrorTexture(nullptr)
	, frameIndex(0)
{
}

void RiftHmd::destroy() {
	if (sceneTextureSet) {
		ovr_DestroySwapTextureSet(hmd, sceneTextureSet);
		sceneTextureSet = nullptr;
	}
	glDeleteRenderbuffers(1, &depthBuffer);
	depthBuffer = 0;
	glDeleteFramebuffers(1, &sceneFrameBuffer);
	sceneFrameBuffer = 0;
	if (hmd) {
		ovr_Destroy(hmd);
		hmd = nullptr;
	}
	ovr_Shutdown();
}

ovrResult RiftHmd::init_tracking() 
{
	if (hmd) return ovrSuccess; // already initialized
	ovrResult result = ovr_Initialize(nullptr);
	if OVR_FAILURE(result)
		return result;
	ovrGraphicsLuid luid;
	ovr_Create(&hmd, &luid);
	ovr_ConfigureTracking(hmd,
			ovrTrackingCap_Orientation | // supported capabilities
			ovrTrackingCap_MagYawCorrection |
			ovrTrackingCap_Position, 
			0); // required capabilities
	return result;
}


ovrResult RiftHmd::init_graphics(int width, int height) 
{
    // NOTE: Initialize OpenGL first (elsewhere), before getting Rift textures here.

	// HMD
	ovrResult result = init_tracking();
	if OVR_FAILURE(result)
		return result;

	// 3D scene
	result = init_scene_texture();
	if OVR_FAILURE(result)
		return result;

	return result;
}

ovrResult RiftHmd::init_scene_texture()
{
	if (sceneTextureSet)
		return ovrSuccess;

    // Configure Stereo settings.
    // Use a single shared texture for simplicity
    // 1bb) Compute texture sizes
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(hmd);
    ovrSizei recommendedTex0Size = ovr_GetFovTextureSize(hmd, ovrEye_Left, 
            hmdDesc.DefaultEyeFov[0], 1.0);
    ovrSizei recommendedTex1Size = ovr_GetFovTextureSize(hmd, ovrEye_Right,
            hmdDesc.DefaultEyeFov[1], 1.0);
    ovrSizei bufferSize;
    bufferSize.w  = recommendedTex0Size.w + recommendedTex1Size.w;
    bufferSize.h = std::max( recommendedTex0Size.h, recommendedTex1Size.h );
    // print "Recommended buffer size = ", bufferSize, bufferSize.w, bufferSize.h
    // NOTE: We need to have set up OpenGL context before this point...
    // 1c) Allocate SwapTextureSets
    ovrResult result = ovr_CreateSwapTextureSetGL(hmd,
            GL_SRGB8_ALPHA8, bufferSize.w, bufferSize.h, &sceneTextureSet);
	if OVR_FAILURE(result)
		return result;

    // Initialize VR structures, filling out description.
    // 1ba) Compute FOV
    ovrEyeRenderDesc eyeRenderDesc[2];
    // ovrVector3f hmdToEyeViewOffset[2];
    eyeRenderDesc[0] = ovr_GetRenderDesc(hmd, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
    eyeRenderDesc[1] = ovr_GetRenderDesc(hmd, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
    hmdToEyeViewOffset[0] = eyeRenderDesc[0].HmdToEyeViewOffset;
    hmdToEyeViewOffset[1] = eyeRenderDesc[1].HmdToEyeViewOffset;

	// Stereo3D Layer for primary 3D scene
    // Initialize our single full screen Fov layer.
    // ovrLayerEyeFov layer;
	sceneLayer.Header.Type      = ovrLayerType_EyeFov;
    sceneLayer.Header.Flags     = ovrLayerFlag_TextureOriginAtBottomLeft; // OpenGL convention;
    sceneLayer.ColorTexture[0]  = sceneTextureSet; // single texture for both eyes;
    sceneLayer.ColorTexture[1]  = sceneTextureSet; // single texture for both eyes;
    sceneLayer.Fov[0]           = eyeRenderDesc[0].Fov;
    sceneLayer.Fov[1]           = eyeRenderDesc[1].Fov;
	sceneLayer.Viewport[0].Pos.x = 0;
	sceneLayer.Viewport[0].Pos.y = 0;
	sceneLayer.Viewport[0].Size.w = bufferSize.w / 2;
	sceneLayer.Viewport[0].Size.h = bufferSize.h;
	sceneLayer.Viewport[1].Pos.x = bufferSize.w / 2;
	sceneLayer.Viewport[1].Pos.y = 0;
	sceneLayer.Viewport[1].Size.w = bufferSize.w / 2;
	sceneLayer.Viewport[1].Size.h = bufferSize.h;

	// create OpenGL framebuffer for rendering to Rift
	glGenFramebuffers(1, &sceneFrameBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneFrameBuffer);
	// color layer will be provided by Rift API at render time...
	// depth buffer
	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bufferSize.w, bufferSize.h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	// clean up
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return result;
}

bool RiftHmd::bindToSceneFrameBufferAndUpdate()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneFrameBuffer);
    ovrFrameTiming ftiming  = ovr_GetFrameTiming(hmd, 0);
    ovrTrackingState hmdState = ovr_GetTrackingState(hmd, ftiming.DisplayMidpointSeconds);
    // print hmdState.HeadPose.ThePose
    ovr_CalcEyePoses(hmdState.HeadPose.ThePose, 
            hmdToEyeViewOffset,
            sceneLayer.RenderPose);
    // Increment to use next texture, just before writing
    // 2d) Advance CurrentIndex within each used texture set to target the next consecutive texture buffer for the following frame.
	int ix = sceneTextureSet->CurrentIndex + 1;
	ix = ix % sceneTextureSet->TextureCount;
    sceneTextureSet->CurrentIndex = ix;
    ovrGLTexture * texture = (ovrGLTexture*) &sceneTextureSet->Textures[ix];
    glFramebufferTexture2D(GL_FRAMEBUFFER, 
            GL_COLOR_ATTACHMENT0, 
            GL_TEXTURE_2D,
			texture->OGL.TexId,
            0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
	GLenum fbStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	GLenum desiredStatus = GL_FRAMEBUFFER_COMPLETE;
	if (fbStatus != GL_FRAMEBUFFER_COMPLETE)
		return false;
	return true;
}

void RiftHmd::paintHudQuad() 
{
	// Place hud relative to torso
	ovrPosef pose = getCurrentEyePose();
	// Convert from Rift camera coordinates to game coordinates
	// float gameYaw = renderer_param.mAngles.Yaw;
	OVR::Quatf hmdRot(pose.Orientation);
	float hmdYaw, hmdPitch, hmdRoll;
	hmdRot.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
	OVR::Quatf yawCorrection(OVR::Vector3f(0, 1, 0), -hmdYaw); // 
	// OVR::Vector3f trans0(pose.Position);
	OVR::Vector3f eyeTrans = yawCorrection.Rotate(pose.Position);

	// Keep HUD fixed relative to the torso, and convert angles to degrees
	float hudPitch = -hmdPitch * 180/3.14159;
	float hudRoll = -hmdRoll * 180/3.14159;
	hmdYaw *= -180/3.14159;

	// But allow hud yaw to vary within a range about torso yaw
	static float hudYaw = 0;
	// shift deviation from camera yaw to range +- 180 degrees
	float dYaw = hmdYaw - hudYaw;
	while (dYaw > 180) dYaw -= 360;
	while (dYaw < -180) dYaw += 360;
	float yawRange = 20;
	if (dYaw < -yawRange) dYaw = -yawRange;
	if (dYaw > yawRange) dYaw = yawRange;
	// Slowly center hud yaw
	float recenterIncrement = 0.010; // degrees
	if (dYaw >= recenterIncrement) dYaw -= recenterIncrement;
	if (dYaw <= -recenterIncrement) dYaw += recenterIncrement;
	hudYaw = hmdYaw - dYaw;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(hudRoll, 0, 0, 1);
	glRotatef(hudPitch, 1, 0, 0);
	glRotatef(dYaw, 0, 1, 0);

	glRotatef(-25, 1, 0, 0); // place hud below horizon

	glTranslatef(-eyeTrans.x, -eyeTrans.y, -eyeTrans.z);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	float hudDistance = 1.5; // meters
	float hudWidth = 1.1 * hudDistance;
	float hudHeight = hudWidth * 3.0f / 4.0f;
	glBegin(GL_TRIANGLE_STRIP);
		glColor4f(1, 1, 1, 0.5);
		glTexCoord2f(0, 1); glVertex3f(-0.5*hudWidth,  0.5*hudHeight, -hudDistance);
		glTexCoord2f(0, 0); glVertex3f(-0.5*hudWidth, -0.5*hudHeight, -hudDistance);
		glTexCoord2f(1, 1); glVertex3f( 0.5*hudWidth,  0.5*hudHeight, -hudDistance);
		glTexCoord2f(1, 0); glVertex3f( 0.5*hudWidth, -0.5*hudHeight, -hudDistance);
	glEnd();
	glEnable(GL_TEXTURE_2D);
}

void RiftHmd::paintCrosshairQuad() 
{
	// Place crosshair relative to head
	ovrPosef pose = getCurrentEyePose();
	// Convert from Rift camera coordinates to game coordinates
	OVR::Quatf hmdRot(pose.Orientation);
	float hmdYaw, hmdPitch, hmdRoll;
	hmdRot.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
	OVR::Vector3f eyeTrans = hmdRot.InverseRotate(pose.Position);

	// Keep crosshair fixed relative to the head, modulo roll, and convert angles to degrees
	float hudRoll = -hmdRoll * 180/3.14159;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(-eyeTrans.x, 0, 0);

	// Correct Roll, but not pitch nor yaw
	glRotatef(hudRoll, 0, 0, 1);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	float hudDistance = 7.0; // meters
	float hudWidth = 1.0/30.0 * hudDistance;
	float hudHeight = hudWidth;
	glBegin(GL_TRIANGLE_STRIP);
		glColor4f(1, 1, 1, 0.5);
		glTexCoord2f(0, 1); glVertex3f(-0.5*hudWidth,  0.5*hudHeight, -hudDistance);
		glTexCoord2f(0, 0); glVertex3f(-0.5*hudWidth, -0.5*hudHeight, -hudDistance);
		glTexCoord2f(1, 1); glVertex3f( 0.5*hudWidth,  0.5*hudHeight, -hudDistance);
		glTexCoord2f(1, 0); glVertex3f( 0.5*hudWidth, -0.5*hudHeight, -hudDistance);
	glEnd();
	glEnable(GL_TEXTURE_2D);
}

void RiftHmd::paintWeaponQuad() 
{
	// Place crosshair relative to head
	ovrPosef pose = getCurrentEyePose();
	// Convert from Rift camera coordinates to game coordinates
	OVR::Quatf hmdRot(pose.Orientation);
	float hmdYaw, hmdPitch, hmdRoll;
	hmdRot.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
	OVR::Vector3f eyeTrans = hmdRot.InverseRotate(pose.Position); // Camera relative X/Y/Z

	// Keep crosshair fixed relative to the head, modulo roll, and convert angles to degrees
	float hudRoll = -hmdRoll * 180/3.14159;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(-eyeTrans.x, -eyeTrans.y, 0); // Ability to parallax look all around weapon

	// Correct Roll, but not pitch nor yaw
	glRotatef(hudRoll, 0, 0, 1);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	float hudDistance = 0.50; // meters, (measured 46 cm to stock of hand weapon)
	float hudWidth = 1.0 * hudDistance; // Adjust for good average weapon size
	float hudHeight = 3.0 / 4.0 * hudWidth;
	glBegin(GL_TRIANGLE_STRIP);
		glColor4f(1, 1, 1, 0.5);
		glTexCoord2f(0, 1); glVertex3f(-0.5*hudWidth,  0.5*hudHeight, -hudDistance);
		glTexCoord2f(0, 0); glVertex3f(-0.5*hudWidth, -0.5*hudHeight, -hudDistance);
		glTexCoord2f(1, 1); glVertex3f( 0.5*hudWidth,  0.5*hudHeight, -hudDistance);
		glTexCoord2f(1, 0); glVertex3f( 0.5*hudWidth, -0.5*hudHeight, -hudDistance);
	glEnd();
	glEnable(GL_TEXTURE_2D);
}

ovrPosef& RiftHmd::setSceneEyeView(int eye, float zNear, float zFar) {
    // Set up eye viewport
    ovrRecti v = sceneLayer.Viewport[eye];
    glViewport(v.Pos.x, v.Pos.y, v.Size.w, v.Size.h);
	glEnable(GL_SCISSOR_TEST);
    glScissor(v.Pos.x, v.Pos.y, v.Size.w, v.Size.h);
    // Get projection matrix for the Rift camera
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    ovrMatrix4f proj = ovrMatrix4f_Projection(sceneLayer.Fov[eye], zNear, zFar,
                    ovrProjection_RightHanded | ovrProjection_ClipRangeOpenGL);
    glMultTransposeMatrixf(&proj.M[0][0]);

    // Get view matrix for the Rift camera
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    currentEyePose = sceneLayer.RenderPose[eye];
	return currentEyePose;
}

ovrResult RiftHmd::submitFrame(float metersPerSceneUnit) {
    // 2c) Call ovr_SubmitFrame, passing swap texture set(s) from the previous step within a ovrLayerEyeFov structure. Although a single layer is required to submit a frame, you can use multiple layers and layer types for advanced rendering. ovr_SubmitFrame passes layer textures to the compositor which handles distortion, timewarp, and GPU synchronization before presenting it to the headset. 
    ovrViewScaleDesc viewScale;
    viewScale.HmdSpaceToWorldScaleInMeters = metersPerSceneUnit;
    viewScale.HmdToEyeViewOffset[0] = hmdToEyeViewOffset[0];
    viewScale.HmdToEyeViewOffset[1] = hmdToEyeViewOffset[1];
	ovrLayerHeader* layerList[1];
	layerList[0] = &sceneLayer.Header;
    ovrResult result = ovr_SubmitFrame(hmd, frameIndex, &viewScale, layerList, 1);
    frameIndex += 1;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return result;
}

void RiftHmd::recenter_pose() {
	ovr_RecenterPose(hmd);
}

static RiftHmd _sharedRiftHmd;
RiftHmd* sharedRiftHmd = &_sharedRiftHmd;
