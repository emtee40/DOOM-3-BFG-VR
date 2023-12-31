#pragma hdrstop

#include"precompiled.h"

#undef strncmp
#undef vsnprintf		
#undef _vsnprintf		

#include "vr.h"
#include "Voice.h"
#include "d3xp\Game_local.h"
#include "sys\win32\win_local.h"
#include "d3xp\physics\Clip.h"
#include "libs\LibOVR\Include\OVR_CAPI_GL.h"
#include "..\renderer\Framebuffer.h"

#define RADIANS_TO_DEGREES(rad) ((float) rad * (float) (180.0 / idMath::PI))

// *** Oculus HMD Variables

idCVar vr_pixelDensity( "vr_pixelDensity", "1.25", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "" );
idCVar vr_vignette( "vr_vignette", "1", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "unused" );
idCVar vr_enable( "vr_enable", "1", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "Enable VR mode. 0 = Disabled 1 = Enabled." );
idCVar vr_FBOscale( "vr_FBOscale", "1.0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_RENDERER, "unused" );
idCVar vr_scale( "vr_scale", "1.0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "unused" );
idCVar vr_useOculusProfile( "vr_useOculusProfile", "1", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "Use Oculus Profile values. 0 = use user defined profile, 1 = use Oculus profile." );
idCVar vr_manualIPDEnable( "vr_manualIPDEnable", "0", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, " Override the HMD provided IPD value with value in vr_manualIPD 0 = disable 1= use manual iPD\n" );
idCVar vr_manualIPD( "vr_manualIPD", "64", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "User defined IPD value in MM" );
idCVar vr_manualHeight( "vr_manualHeight", "70", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "User defined player height in inches" );
idCVar vr_minLoadScreenTime( "vr_minLoadScreenTime", "6000", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Min time to display load screens in ms.", 0.0f, 10000.0f );

idCVar vr_clipPositional( "vr_clipPositional", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Clip positional tracking movement\n. 1 = Clip 0 = No clipping.\n" );

idCVar vr_armIKenable( "vr_armIKenable", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Enable IK on arms when using motion controls and player body is visible.\n 1 = Enabled 0 = disabled\n" );
idCVar vr_weaponHand( "vr_weaponHand", "0", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "Which hand holds weapon.\n 0 = Right hand\n 1 = Left Hand\n", 0, 1 );

//flashlight cvars

idCVar vr_flashlightMode( "vr_flashlightMode", "3", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_GAME, "Flashlight mount.\n0 = Body\n1 = Head\n2 = Gun\n3= Hand ( if motion controls available.)" );

//tweak flash position when aiming with hydra

idCVar vr_flashlightBodyPosX( "vr_flashlightBodyPosX", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight vertical offset for body mount." );
idCVar vr_flashlightBodyPosY( "vr_flashlightBodyPosY", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight horizontal offset for body mount." );
idCVar vr_flashlightBodyPosZ( "vr_flashlightBodyPosZ", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight forward offset for body mount." );

idCVar vr_flashlightHelmetPosX( "vr_flashlightHelmetPosX", "6", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight vertical offset for helmet mount." );
idCVar vr_flashlightHelmetPosY( "vr_flashlightHelmetPosY", "-6", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight horizontal offset for helmet mount." );
idCVar vr_flashlightHelmetPosZ( "vr_flashlightHelmetPosZ", "-20", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Flashlight forward offset for helmet mount." );

idCVar vr_offHandPosX( "vr_offHandPosX", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "X position for off hand when not using motion controls." );
idCVar vr_offHandPosY( "vr_offHandPosY", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Y position for off hand when not using motion controls." );
idCVar vr_offHandPosZ( "vr_offHandPosZ", "0", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Z position for off hand when not using motion controls." );


idCVar vr_forward_keyhole( "vr_forward_keyhole", "11.25", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, "Forward movement keyhole in deg. If view is inside body direction +/- this value, forward movement is in view direction, not body direction" );

idCVar vr_PDAscale( "vr_PDAscale", "3", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_GAME, " unused" );
idCVar vr_PDAfixLocation( "vr_PDAfixLocation", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Fix PDA position in space in front of player\n instead of holding in hand." );

idCVar vr_weaponPivotOffsetForward( "vr_weaponPivotOffsetForward", "3", CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "" );
idCVar vr_weaponPivotOffsetHorizontal( "vr_weaponPivotOffsetHorizontal", "0", CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "" );
idCVar vr_weaponPivotOffsetVertical( "vr_weaponPivotOffsetVertical", "0", CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "" );
idCVar vr_weaponPivotForearmLength( "vr_weaponPivotForearmLength", "16", CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT, "" );;

idCVar vr_guiScale( "vr_guiScale", "1", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "scale reduction factor for full screen menu/pda scale in VR", 0.0001f, 1.0f ); //koz allow scaling of full screen guis/pda
idCVar vr_guiSeparation( "vr_guiSeparation", ".01", CVAR_FLOAT | CVAR_ARCHIVE, " Screen separation value for fullscreen guis." );

idCVar vr_guiMode( "vr_guiMode", "2", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Gui interaction mode.\n 0 = Weapon aim as cursor\n 1 = Look direction as cursor\n 2 = Touch screen\n" );

idCVar vr_hudScale( "vr_hudScale", "1.0", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud scale", 0.1f, 2.0f );
idCVar vr_hudPosHor( "vr_hudPosHor", "0", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud Horizontal offset in inches" );
idCVar vr_hudPosVer( "vr_hudPosVer", "7", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud Vertical offset in inches" );
idCVar vr_hudPosDis( "vr_hudPosDis", "32", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud Distance from view in inches" );
idCVar vr_hudPosAngle( "vr_hudPosAngle", "30", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Hud View Angle" );
idCVar vr_hudPosLock( "vr_hudPosLock", "1", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Lock Hud to:  0 = Face, 1 = Body" );


idCVar vr_hudType( "vr_hudType", "2", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "VR Hud Type. 0 = Disable.\n1 = Full\n2=Look Activate", 0, 2 );
idCVar vr_hudRevealAngle( "vr_hudRevealAngle", "48", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "HMD pitch to reveal HUD in look activate mode." );
idCVar vr_hudTransparency( "vr_hudTransparency", "1", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, " Hud transparency. 0.0 = Invisible thru 1.0 = full", 0.0, 100.0 );
idCVar vr_hudOcclusion( "vr_hudOcclusion", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, " Hud occlusion. 0 = Objects occlude HUD, 1 = No occlusion " );
idCVar vr_hudHealth( "vr_hudHealth", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show Armor/Health in Hud." );
idCVar vr_hudAmmo( "vr_hudAmmo", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show Ammo in Hud." );
idCVar vr_hudPickUps( "vr_hudPickUps", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show item pick ups in Hud." );
idCVar vr_hudTips( "vr_hudTips", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show tips Hud." );
idCVar vr_hudLocation( "vr_hudLocation", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show player location in Hud." );
idCVar vr_hudObjective( "vr_hudObjective", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show objectives in Hud." );
idCVar vr_hudStamina( "vr_hudStamina", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show stamina in Hud." );
idCVar vr_hudPills( "vr_hudPills", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show weapon pills in Hud." );
idCVar vr_hudComs( "vr_hudComs", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show communications in Hud." );
idCVar vr_hudWeap( "vr_hudWeap", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show weapon pickup/change icons in Hud." );
idCVar vr_hudNewItems( "vr_hudNewItems", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show new items acquired in Hud." );
idCVar vr_hudFlashlight( "vr_hudFlashlight", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Show flashlight in Hud." );
idCVar vr_hudLowHealth( "vr_hudLowHealth", "0", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, " 0 = Disable, otherwise force hud if heath below this value." );

idCVar vr_talkMode("vr_talkMode", "2", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Talk to NPC 0 = buttons, 1 = buttons or voice, 2 = voice only, 3 = voice no cursor", 0, 3);
idCVar vr_tweakTalkCursor( "vr_tweakTalkCursor", "25", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Tweak talk cursor y pos in VR. % val", 0, 99 );

idCVar vr_wristStatMon( "vr_wristStatMon", "1", CVAR_INTEGER | CVAR_ARCHIVE, "Use wrist status monitor. 0 = Disable 1 = Right Wrist 2 = Left Wrist " );

// koz display windows monitor name in the resolution selection menu, helpful to ID which is the rift if using extended mode
idCVar vr_listMonitorName( "vr_listMonitorName", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "List monitor name with resolution." );

idCVar vr_viewModelArms( "vr_viewModelArms", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, " Dont change this, will be removed. Display arms on view models in VR" );
idCVar vr_disableWeaponAnimation( "vr_disableWeaponAnimation", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Disable weapon animations in VR. ( 1 = disabled )" );
idCVar vr_headKick( "vr_headKick", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Damage can 'kick' the players view. 0 = Disabled in VR." );
//idCVar vr_showBody( "vr_showBody", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Dont change this! Will be removed shortly, modifying will cause the player to have extra hands." );
idCVar vr_joystickMenuMapping( "vr_joystickMenuMapping", "1", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, " Use alternate joy mapping\n in menus/PDA.\n 0 = D3 Standard\n 1 = VR Mode.\n(Both joys can nav menus,\n joy r/l to change\nselect area in PDA." );


idCVar	vr_deadzonePitch( "vr_deadzonePitch", "90", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Vertical Aim Deadzone", 0, 180 );
idCVar	vr_deadzoneYaw( "vr_deadzoneYaw", "30", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Horizontal Aim Deadzone", 0, 180 );
idCVar	vr_comfortDelta( "vr_comfortDelta", "10", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Comfort Mode turning angle ", 0, 180 );

//idCVar	vr_interactiveCinematic( "vr_interactiveCinematic", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Interactive cinematics in VR ( no camera )" );

idCVar	vr_headingBeamWidth( "vr_headingBeamWidth", "12.0", CVAR_FLOAT | CVAR_ARCHIVE, "heading beam width" ); // Koz default was 2, IMO too big in VR.
idCVar	vr_headingBeamLength( "vr_headingBeamLength", "96", CVAR_FLOAT | CVAR_ARCHIVE, "heading beam length" ); // koz default was 250, but was to short in VR.  Length will be clipped if object is hit, this is max length for the hit trace. 
idCVar	vr_headingBeamMode( "vr_headingBeamMode", "3", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "0 = disabled, 1 = solid, 2 = arrows, 3 = scrolling arrows" );

idCVar	vr_weaponSight( "vr_weaponSight", "0", CVAR_INTEGER | CVAR_ARCHIVE, "Weapon Sight.\n 0 = Lasersight\n 1 = Red dot\n 2 = Circle dot\n 3 = Crosshair\n" );
idCVar	vr_weaponSightToSurface( "vr_weaponSightToSurface", "0", CVAR_INTEGER | CVAR_ARCHIVE, "Map sight to surface. 0 = Disabled 1 = Enabled\n" );

idCVar	vr_motionWeaponPitchAdj( "vr_motionWeaponPitchAdj", "40", CVAR_FLOAT | CVAR_ARCHIVE, "Weapon controller pitch adjust" );
idCVar	vr_motionFlashPitchAdj( "vr_motionFlashPitchAdj", "40", CVAR_FLOAT | CVAR_ARCHIVE, "Flash controller pitch adjust" );

idCVar	vr_nodalX( "vr_nodalX", "-3", CVAR_FLOAT | CVAR_ARCHIVE, "Forward offset from eyes to neck" );
idCVar	vr_nodalZ( "vr_nodalZ", "-6", CVAR_FLOAT | CVAR_ARCHIVE, "Vertical offset from neck to eye height" );


idCVar vr_vcx( "vr_vcx", "-3.5", CVAR_FLOAT | CVAR_ARCHIVE, "Controller X offset to handle center" ); // these values work for steam
idCVar vr_vcy( "vr_vcy", "0", CVAR_FLOAT | CVAR_ARCHIVE, "Controller Y offset to handle center" );
idCVar vr_vcz( "vr_vcz", "-.5", CVAR_FLOAT | CVAR_ARCHIVE, "Controller Z offset to handle center" );

idCVar vr_mountx( "vr_mountx", "0", CVAR_FLOAT | CVAR_ARCHIVE, "If motion controller mounted on object, X offset from controller to object handle.\n (Eg controller mounted on Topshot)" );
idCVar vr_mounty( "vr_mounty", "0", CVAR_FLOAT | CVAR_ARCHIVE, "If motion controller mounted on object, Y offset from controller to object handle.\n (Eg controller mounted on Topshot)" );
idCVar vr_mountz( "vr_mountz", "0", CVAR_FLOAT | CVAR_ARCHIVE, "If motion controller mounted on object, Z offset from controller to object handle.\n (Eg controller mounted on Topshot)" );

idCVar vr_mountedWeaponController( "vr_mountedWeaponController", "0", CVAR_BOOL | CVAR_ARCHIVE, "If physical controller mounted on object (eg topshot), enable this to apply mounting offsets\n0=disabled 1 = enabled" );

idCVar vr_3dgui( "vr_3dgui", "1", CVAR_BOOL | CVAR_ARCHIVE, "3d effects for in game guis. 0 = disabled 1 = enabled\n" );
idCVar vr_shakeAmplitude( "vr_shakeAmplitude", "1.0", CVAR_FLOAT | CVAR_ARCHIVE, "Screen shake amplitude 0.0 = disabled to 1.0 = full\n", 0.0f, 1.0f );


idCVar vr_controllerStandard( "vr_controllerStandard", "0", CVAR_INTEGER | CVAR_ARCHIVE, "If 1, use standard controller, not motion controllers\nRestart after changing\n" );

idCVar vr_padDeadzone( "vr_padDeadzone", ".25", CVAR_FLOAT | CVAR_ARCHIVE, "Deadzone for steam pads.\n 0.0 = no deadzone 1.0 = dead\n" );
idCVar vr_padToButtonThreshold( "vr_padToButtonThreshold", ".7", CVAR_FLOAT | CVAR_ARCHIVE, "Threshold value for pad contact\n to register as button press\n .1 high sensitiveity thru\n .99 low sensitivity" );
idCVar vr_knockBack( "vr_knockBack", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_GAME, "Enable damage knockback in VR. 0 = Disabled, 1 = Enabled" );
idCVar vr_walkSpeedAdjust( "vr_walkSpeedAdjust", "-20", CVAR_FLOAT | CVAR_ARCHIVE, "Player walk speed adjustment in VR. (slow down default movement)" );

idCVar vr_wipPeriodMin( "vr_wipPeriodMin", "10.0", CVAR_FLOAT | CVAR_ARCHIVE, "" );
idCVar vr_wipPeriodMax( "vr_wipPeriodMax", "2000.0", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_wipVelocityMin( "vr_wipVelocityMin", ".05", CVAR_FLOAT | CVAR_ARCHIVE, "" );
idCVar vr_wipVelocityMax( "vr_wipVelocityMax", "2.0", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_headbbox( "vr_headbbox", "10.0", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_pdaPosX( "vr_pdaPosX", "20", CVAR_FLOAT | CVAR_ARCHIVE, "" );
idCVar vr_pdaPosY( "vr_pdaPosY", "0", CVAR_FLOAT | CVAR_ARCHIVE, "" );
idCVar vr_pdaPosZ( "vr_pdaPosZ", "-11", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_pdaPitch( "vr_pdaPitch", "30", CVAR_FLOAT | CVAR_ARCHIVE, "" );

idCVar vr_movePoint( "vr_movePoint", "0", CVAR_INTEGER | CVAR_ARCHIVE, "If enabled, move in the direction the off hand is pointing." );
idCVar vr_moveClick( "vr_moveClick", "0", CVAR_INTEGER | CVAR_ARCHIVE, " 0 = Normal movement.\n 1 = Click and hold to walk, run button to run.\n 2 = Click to start walking, then touch only. Run btn to run.\n 3 = Click to start walking, hold click to run.\n 4 = Click to start walking, then click toggles run\n" );
idCVar vr_playerBodyMode( "vr_playerBodyMode", "0", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Player body mode:\n0 = Display full body\n1 = Just Hands \n2 = Weapons only\n" );
idCVar vr_bodyToMove( "vr_bodyToMove", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Lock body orientaion to movement direction." );

idCVar vr_crouchMode( "vr_crouchMode", "0", CVAR_INTEGER | CVAR_GAME | CVAR_ARCHIVE, "Crouch Mode:\n 0 = Full motion crouch (In game matches real life)\n 1 = Crouch anim triggered by smaller movement." );
idCVar vr_crouchTriggerDist( "vr_crouchTriggerDist", "10", CVAR_FLOAT | CVAR_ARCHIVE, " Distance ( in inches ) player must crouch in real life to toggle crouch\n" );

idCVar vr_frameCheck( "vr_frameCheck", "0", CVAR_INTEGER | CVAR_ARCHIVE, "0 = bypass frame check" );

idCVar vr_forceOculusAudio( "vr_forceOculusAudio", "1", CVAR_BOOL | CVAR_ARCHIVE, "Request openAL to search for Rift headphones instead of default device\nFails to default device if rift not found." );
idCVar vr_stereoMirror( "vr_stereoMirror", "1", CVAR_BOOL | CVAR_ARCHIVE, "Render mirror window with stereo views. 0 = Mono , 1 = Stereo Warped" );
// Koz end
//===================================================================

int fboWidth;
int fboHeight;

iVr vrCom;
iVr* commonVr = &vrCom;

iVoice voice;
iVoice* commonVoice = &voice;

/*
====================
R_MakeFBOImage
//Koz deleteme, using renderbuffers instead of textures.
====================
*/
static void R_MakeFBOImage( idImage* image )
{
	idImageOpts	opts;
	opts.width = fboWidth;
	opts.height = fboHeight;
	opts.numLevels = 1;
	opts.format = FMT_RGBA8;
	image->AllocImage( opts, TF_LINEAR, TR_CLAMP );
}

/*
==============
iVr::iVr()
==============
*/
iVr::iVr()
{
	hasHMD = false;
	hasOculusRift = false;
	VR_GAME_PAUSED = false;
	PDAforcetoggle = false;
	PDAforced = false;
	PDArising = false;
	gameSavingLoading = false;
	forceLeftStick = true;	// start the PDA in the left menu.
	pdaToggleTime = Sys_Milliseconds();
	lastSaveTime = Sys_Milliseconds();
	wasSaved = false;
	wasLoaded = false;

	PDAclipModelSet = false;
	useFBO = false;
	VR_USE_MOTION_CONTROLS = 0;

	scanningPDA = false;

	vrIsBackgroundSaving = false;

	oculusIPD = 64.0f;
	oculusHeight = 72.0f;

	manualIPD = 64.0f;
	manualHeight = 72.0f;

	hmdPositionTracked = false;

	
	lastPostFrame = 0;


	lastViewOrigin = vec3_zero;
	lastViewAxis = mat3_identity;

	lastCenterEyeOrigin = vec3_zero;
	lastCenterEyeAxis = mat3_identity;

	bodyYawOffset = 0.0f;
	lastHMDYaw = 0.0f;
	lastHMDPitch = 0.0f;
	lastHMDRoll = 0.0f;
	lastHMDViewOrigin = vec3_zero;
	lastHMDViewAxis = mat3_identity;
	
	motionMoveDelta = vec3_zero;
	motionMoveVelocity = vec3_zero;
	leanOffset = vec3_zero;

	
	chestDefaultDefined = false;

	currentFlashlightPosition = FLASH_BODY;

	handInGui = false;

	handRoll[0] = 0.0f;
	handRoll[1] = 0.0f;

	angles[3] = { 0.0f };

	swfRenderMode = RENDERING_NORMAL;

	isWalking = false;

	hmdBodyTranslation = vec3_zero;

	VR_AAmode = 0;

	independentWeaponYaw = 0;
	independentWeaponPitch = 0;

	playerDead = false;

	hmdWidth = 0;
	hmdHeight = 0;

	primaryFBOWidth = 0;
	primaryFBOHeight = 0;
	hmdHz = 90;
	hmdSession = nullptr;
	ovrLuid.Reserved[0] = { 0 };
	hmdFovX = 0.0f;
	hmdFovY = 0.0f;
	hmdPixelScale = 1.0f;
	hmdAspect = 1.0f;
	oculusSwapChain[0] = nullptr;
	oculusSwapChain[1] = nullptr;
	oculusFboId = 0;
	ocululsDepthTexID = 0;
	oculusMirrorFboId = 0;
	oculusMirrorTexture = 0;
	mirrorTexId = 0;
	mirrorW = 0;
	mirrorH = 0;

	hmdEyeImage[0] = 0;
	hmdEyeImage[1] = 0;
	hmdCurrentRender[0] = 0;
	hmdCurrentRender[1] = 0;

	// wip stuff
	// wip stuff
	wipNumSteps = 0;
	wipStepState = 0;
	wipLastPeriod = 0;
	wipCurrentDelta = 0.0f;
	wipCurrentVelocity = 0.0f;

	wipTotalDelta = 0.0f;
	wipLastAcces = 0.0f;
	wipAvgPeriod = 0.0f;
	wipTotalDeltaAvg = 0.0f;

	hmdFrameTime = 0;
	
	lastRead = 0;
	currentRead = 0;
	updateScreen = false;

	motionControlType = MOTION_NONE;

	bodyMoveAng = 0.0f;

	currentFlashMode = vr_flashlightMode.GetInteger();
	renderingSplash = true;
	
}


/*
==============
iVr::HMDInit
==============
*/

void iVr::HMDInit( void )
{
	hasHMD = false;
	hasOculusRift = false;
	game->isVR = false;
	// Oculus HMD Initialization
	ovrResult result = ovr_Initialize( nullptr );

	if ( OVR_FAILURE( result ) )
	{
		common->Printf( "\nOculus Rift not detected.\n" );
		return;
	}


	common->Printf( "ovr_Initialize was successful.\n" );
	result = ovr_Create( &hmdSession, &ovrLuid );

	if ( OVR_FAILURE( result ) )
	{
		common->Printf( "\nFailed to initialize Oculus Rift.\n" );
		ovr_Shutdown();
		return;
	}
	
	hmdDesc = ovr_GetHmdDesc( hmdSession );
	hasOculusRift = true;
	hasHMD = true;
	
	//ovrSizei resoultion = hmdDesc.Resolution;

	common->Printf( "\n\nOculus Rift HMD Initialized\n" );
	//ovr_RecenterPose( hmdSession ); // lets start looking forward.
	ovr_RecenterTrackingOrigin( hmdSession );
	hmdWidth = hmdDesc.Resolution.w;
	hmdHeight = hmdDesc.Resolution.h;
	hmdHz = hmdDesc.DisplayRefreshRate;
	com_engineHz.SetInteger( hmdHz );
	common->Printf( "Hmd: %s .\n", hmdDesc.ProductName );
	common->Printf( "Hmd HZ %d, width %d, height %d\n", hmdHz, hmdWidth, hmdHeight );
	
	ovr_GetAudioDeviceOutGuid( &oculusGuid );
	ovr_GetAudioDeviceOutGuidStr( oculusGuidStr );

	common->Printf( "Oculus sound guid " );
	for ( int c = 0; c < 128; c++ )
	{
		common->Printf( "%c", oculusGuidStr[c] );
	}
	common->Printf( "Oculus sound guid\n" );
	/*
	vr::TrackedDeviceIndex_t deviceLeft,deviceRight;

	deviceLeft = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_LeftHand );
	deviceRight = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_RightHand );

	common->Printf( "Left Controller %d right Controller %d\n", deviceLeft, deviceRight );

	if ( deviceLeft != -1 || deviceRight != -1  )
	{
	common->Printf( "Tracked controllers detected. MOTION CONTROLS ENABLED\n" );
	VR_USE_MOTION_CONTROLS = true;
	}
	else
	{
	VR_USE_MOTION_CONTROLS = false;
	}
	*/

	VR_USE_MOTION_CONTROLS = false;

	int ctrlrs = ovr_GetConnectedControllerTypes( hmdSession );
	if ( (ctrlrs && 3) != 0 )
	{
		VR_USE_MOTION_CONTROLS = true;
		motionControlType = MOTION_OCULUS;
	}

	
	common->Printf( "\n\n HMD Initialized\n" );

	hasOculusRift = true;
	hasHMD = true;
	common->Printf( "VR_USE_MOTION_CONTROLS Final = %d\n", VR_USE_MOTION_CONTROLS );
	
}


/*
==============
iVr::HMDShutdown
==============
*/

void iVr::HMDShutdown( void )
{
	ovr_DestroyTextureSwapChain( hmdSession, oculusSwapChain[0] );
	ovr_DestroyTextureSwapChain( hmdSession, oculusSwapChain[1] );

	ovr_Destroy( hmdSession );
	ovr_Shutdown();
	return;
}

/*
==============
iVr::HMDInitializeDistortion
==============
*/

void iVr::HMDInitializeDistortion()
{

	if ( !commonVr->hmdSession || !commonVr->hasOculusRift || !vr_enable.GetBool() )
	{
		game->isVR = false;
		return;
	}

	game->isVR = true;
	common->Printf( "VR Mode ENABLED.\n" );

	if ( !glConfig.framebufferObjectAvailable )
	{
		common->Error( "Framebuffer object not available.  Framebuffer support required for VR.\n" );
	}

	bool fboCreated = false;


	int eye = 0;
	for ( int eye = 0; eye < 2; eye++ )
	{

		hmdEye[eye].eyeFov = commonVr->hmdDesc.DefaultEyeFov[eye];
		hmdEye[eye].eyeRenderDesc = ovr_GetRenderDesc( commonVr->hmdSession, (ovrEyeType)eye, hmdEye[eye].eyeFov );

		
		ovrMatrix4f pEye = ovrMatrix4f_Projection( hmdEye[eye].eyeRenderDesc.Fov, 1.0f, -0.9999999999f, true ); 
		int x, y;

		for ( x = 0; x < 4; x++ )
		{
			for ( y = 0; y < 4; y++ )
			{
				hmdEye[eye].projectionRift[y * 4 + x] = pEye.M[x][y];						
			}
		}

		hmdEye[eye].projection.x.scale = 2.0f / (hmdEye[eye].eyeFov.LeftTan + hmdEye[eye].eyeFov.RightTan);
		hmdEye[eye].projection.x.offset = (hmdEye[eye].eyeFov.LeftTan - hmdEye[eye].eyeFov.RightTan) * hmdEye[eye].projection.x.scale * 0.5f;
		hmdEye[eye].projection.y.scale = 2.0f / (hmdEye[eye].eyeFov.UpTan + hmdEye[eye].eyeFov.DownTan);
		hmdEye[eye].projection.y.offset = (hmdEye[eye].eyeFov.UpTan - hmdEye[eye].eyeFov.DownTan) * hmdEye[eye].projection.y.scale * 0.5f;

		hmdEye[eye].viewOffset.x = hmdEye[eye].eyeRenderDesc.HmdToEyeOffset.x;
		hmdEye[eye].viewOffset.y = hmdEye[eye].eyeRenderDesc.HmdToEyeOffset.y;
		hmdEye[eye].viewOffset.z = hmdEye[eye].eyeRenderDesc.HmdToEyeOffset.z;

		common->Printf( "EYE %d px.scale %f, px.offset %f, py.scale %f, py.offset %f\n", eye, hmdEye[eye].projection.x.scale, hmdEye[eye].projection.x.offset, hmdEye[eye].projection.y.scale, hmdEye[eye].projection.y.offset );
		common->Printf( "EYE %d viewoffset viewadjust x %f y %f z %f\n", eye, hmdEye[eye].viewOffset.x, hmdEye[eye].viewOffset.y, hmdEye[eye].viewOffset.z );

		common->Printf( "EYE %d HmdToEyeOffset x %f y %f z %f\n", eye, hmdEye[eye].eyeRenderDesc.HmdToEyeOffset.x, hmdEye[eye].eyeRenderDesc.HmdToEyeOffset.y, hmdEye[eye].eyeRenderDesc.HmdToEyeOffset.z );

		ovrSizei rendertarget;
		ovrRecti viewport = { 0, 0, 0, 0 };

		rendertarget = ovr_GetFovTextureSize( commonVr->hmdSession, (ovrEyeType)eye, commonVr->hmdEye[eye].eyeFov, vr_pixelDensity.GetFloat() ); // make sure both eyes render to the same size target
		hmdEye[eye].renderTarget.h = rendertarget.h; // koz was height?
		hmdEye[eye].renderTarget.w = rendertarget.w;
		common->Printf( "Eye %d Rendertaget Width x Height = %d x %d\n", eye, rendertarget.w, rendertarget.h );

		if ( !fboCreated )
		{
			common->Printf( "Generating FBOs.\n" );
			common->Printf( "Requested pixel density = %f \n", vr_pixelDensity.GetFloat() );
			primaryFBOWidth = rendertarget.w;
			primaryFBOHeight = rendertarget.h;
			fboWidth = rendertarget.w;
			fboHeight = rendertarget.h;


			if ( !fboCreated )
			{ // create the FBOs if needed.

				VR_AAmode = r_multiSamples.GetInteger() == 0 ? VR_AA_NONE : VR_AA_MSAA;
			
				common->Printf( "vr_FBOAAmode %d r_multisamples %d\n", VR_AAmode, r_multiSamples.GetInteger() );
				
				/*
				if ( VR_AAmode == VR_AA_FXAA )
				{// enable FXAA

					VR_AAmode = VR_AA_NONE;

				}
				*/

				if ( VR_AAmode == VR_AA_MSAA )
				{	// enable MSAA
					GL_CheckErrors();

					common->Printf( "Creating %d x %d MSAA framebuffer\n", rendertarget.w, rendertarget.h );
					globalFramebuffers.primaryFBO = new Framebuffer( "_primaryFBO", rendertarget.w, rendertarget.h, true ); // koz
					common->Printf( "Adding Depth/Stencil attachments to MSAA framebuffer\n" );
					globalFramebuffers.primaryFBO->AddDepthStencilBuffer( GL_DEPTH24_STENCIL8 );
					common->Printf( "Adding color attachment to MSAA framebuffer\n" );
					globalFramebuffers.primaryFBO->AddColorBuffer( GL_RGBA, 0 );

					int status = globalFramebuffers.primaryFBO->Check();
					globalFramebuffers.primaryFBO->Error( status );

					common->Printf( "Creating resolve framebuffer\n" );
					globalFramebuffers.resolveFBO = new Framebuffer( "_resolveFBO", rendertarget.w, rendertarget.h, false ); // koz
					common->Printf( "Adding Depth/Stencil attachments to framebuffer\n" );
					globalFramebuffers.resolveFBO->AddDepthStencilBuffer( GL_DEPTH24_STENCIL8 );
					common->Printf( "Adding color attachment to framebuffer\n" );
					globalFramebuffers.resolveFBO->AddColorBuffer( GL_RGBA, 0 );

					status = globalFramebuffers.resolveFBO->Check();
					globalFramebuffers.resolveFBO->Error( status );

					fboWidth = globalFramebuffers.primaryFBO->GetWidth();// rendertarget.w;
					fboHeight = globalFramebuffers.primaryFBO->GetHeight();
					common->Printf( "Globalframebuffer w x h  = %d x %d\n", fboWidth, fboHeight );
					rendertarget.w = fboWidth;
					rendertarget.h = fboHeight;

					if ( status = GL_FRAMEBUFFER_COMPLETE )
					{
						useFBO = true;
						fboCreated = true;
					}
					else
					{
						useFBO = false;
						fboCreated = false;
					}

				}

				if ( !fboCreated /*!VR_FBO.valid*/ )
				{ // either AA disabled or AA buffer creation failed. Try creating unaliased FBOs.

					//primaryFBOimage = globalImages->ImageFromFunction( "_primaryFBOimage", R_MakeFBOImage );
					common->Printf( "Creating framebuffer\n" );
					globalFramebuffers.primaryFBO = new Framebuffer( "_primaryFBO", rendertarget.w, rendertarget.h, false ); // koz
					common->Printf( "Adding Depth/Stencil attachments to framebuffer\n" );
					globalFramebuffers.primaryFBO->AddDepthStencilBuffer( GL_DEPTH24_STENCIL8 );
					common->Printf( "Adding color attachment to framebuffer\n" );
					globalFramebuffers.primaryFBO->AddColorBuffer( GL_RGBA8, 0 );

					int status = globalFramebuffers.primaryFBO->Check();
					globalFramebuffers.primaryFBO->Error( status );

					if ( status = GL_FRAMEBUFFER_COMPLETE )
					{
						useFBO = true;
						fboCreated = true;
					}
					else
					{
						useFBO = false;
						fboCreated = false;
					}
				}
			}
		}

		if ( !useFBO ) { // not using FBO's, will render to default framebuffer (screen) 

			rendertarget.w = renderSystem->GetNativeWidth() / 2;
			rendertarget.h = renderSystem->GetNativeHeight();
			hmdEye[eye].renderTarget = rendertarget;


		}

		viewport.Size.w = rendertarget.w;
		viewport.Size.h = rendertarget.h;

		globalImages->hudImage->Resize( rendertarget.w, rendertarget.h );
		globalImages->pdaImage->Resize( rendertarget.w, rendertarget.h );
		globalImages->currentRenderImage->Resize( rendertarget.w, rendertarget.h );
		globalImages->currentDepthImage->Resize( rendertarget.w, rendertarget.h );

		common->Printf( "pdaImage size %d %d\n", globalImages->pdaImage->GetUploadWidth(), globalImages->pdaImage->GetUploadHeight() );
		common->Printf( "Hudimage size %d %d\n", globalImages->hudImage->GetUploadWidth(), globalImages->hudImage->GetUploadHeight() );



	}

	// total IPD in mm
	oculusIPD = ( fabs(hmdEye[0].viewOffset.x) + fabs(hmdEye[1].viewOffset.x) ) * 1000.0f;
	common->Printf( "Oculus IPD : %f\n", oculusIPD );
	// calculate fov for engine
	float combinedTanHalfFovHorizontal = std::max( std::max( hmdEye[0].eyeFov.LeftTan, hmdEye[0].eyeFov.RightTan ), std::max( hmdEye[1].eyeFov.LeftTan, hmdEye[1].eyeFov.RightTan ) );
	float combinedTanHalfFovVertical = std::max( std::max( hmdEye[0].eyeFov.UpTan, hmdEye[0].eyeFov.DownTan ), std::max( hmdEye[1].eyeFov.UpTan, hmdEye[1].eyeFov.DownTan ) );
	float horizontalFullFovInRadians = 2.0f * atanf( combinedTanHalfFovHorizontal );

	hmdFovX = RAD2DEG( horizontalFullFovInRadians );
	hmdFovY = RAD2DEG( 2.0 * atanf( combinedTanHalfFovVertical ) );
	hmdAspect = combinedTanHalfFovHorizontal / combinedTanHalfFovVertical;
	hmdPixelScale = 1;//ovrScale * vid.width / (float) hmd->Resolution.w;	

	hmdEye[0].renderTarget.w = globalFramebuffers.primaryFBO->GetWidth();
	hmdEye[0].renderTarget.h = globalFramebuffers.primaryFBO->GetHeight();


	hmdEye[1].renderTarget = hmdEye[0].renderTarget;

	common->Printf( "Init Hmd FOV x,y = %f , %f. Aspect = %f, PixelScale = %f\n", hmdFovX, hmdFovY, hmdAspect, hmdPixelScale );
	common->Printf( "Creating oculus texture set width = %d height = %d.\n", hmdEye[0].renderTarget.w, hmdEye[0].renderTarget.h );

	ovrTextureSwapChainDesc desc = {};
	desc.Type = ovrTexture_2D;
	desc.ArraySize = 1;
	desc.Width = hmdEye[0].renderTarget.w;
	desc.Height = hmdEye[0].renderTarget.h;
	desc.MipLevels = 1;
	desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleCount = 1;
	desc.StaticImage = ovrFalse;


	// create the swap texture sets 
	if ( ovr_CreateTextureSwapChainGL( hmdSession, &desc, &oculusSwapChain[0] ) != ovrSuccess ||
		ovr_CreateTextureSwapChainGL( hmdSession, &desc, &oculusSwapChain[1] ) != ovrSuccess )
	{
		common->Warning( "iVr::HMDInitializeDistortion unable to create OVR swap texture set.\n VR mode is DISABLED.\n" );
		game->isVR = false;

	}

	unsigned int texId = 0;
	int length = 0;

	for ( int j = 0; j < 2; j++ )
	{
		ovr_GetTextureSwapChainLength( hmdSession, oculusSwapChain[j], &length );
		for ( int i = 0; i < length; ++i )
		{
			ovr_GetTextureSwapChainBufferGL( hmdSession, oculusSwapChain[j], 0, &texId );
			//oculusSwapChainTexId[j] = texId;

			glBindTexture( GL_TEXTURE_2D, texId );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		}
	}

	ovr_CommitTextureSwapChain( hmdSession, oculusSwapChain[0] );
	ovr_CommitTextureSwapChain( hmdSession, oculusSwapChain[1] );

	glGenFramebuffers( 1, &oculusFboId );
	glGenTextures( 1, &ocululsDepthTexID );

	glBindTexture( GL_TEXTURE_2D, ocululsDepthTexID );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, hmdEye[0].renderTarget.w, hmdEye[0].renderTarget.h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL );


	//int ww = glConfig.nativeScreenWidth / 2;
	//int wh = glConfig.nativeScreenHeight / 2;

	int ww = hmdDesc.Resolution.w / 2;
	int wh = hmdDesc.Resolution.h / 2;

	ovrMirrorTextureDesc mirrorDesc;
	memset( &mirrorDesc, 0, sizeof( mirrorDesc ) );
	mirrorDesc.Width = ww;
	mirrorDesc.Height = wh;
	mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	mirrorW = ww;
	mirrorH = wh;

	common->Printf( "Creating oculus mirror texture %d x %d\n", ww, wh );
	ovr_CreateMirrorTextureGL( hmdSession, &mirrorDesc, &oculusMirrorTexture );
	ovr_GetMirrorTextureBufferGL( hmdSession, oculusMirrorTexture, &mirrorTexId );
	glGenFramebuffers( 1, &oculusMirrorFboId );
	glBindFramebuffer( GL_READ_FRAMEBUFFER, oculusMirrorFboId );
	glFramebufferTexture2D( GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexId, 0 );
	glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0 );
	glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );

	wglSwapIntervalEXT( 0 );

	oculusLayer.Header.Type = ovrLayerType_EyeFov;
	oculusLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
	oculusLayer.ColorTexture[0] = oculusSwapChain[0];
	oculusLayer.ColorTexture[1] = oculusSwapChain[1];
	oculusLayer.Fov[0] = hmdEye[0].eyeRenderDesc.Fov;
	oculusLayer.Fov[1] = hmdEye[1].eyeRenderDesc.Fov;
	oculusLayer.Viewport[0].Pos.x = 0;
	oculusLayer.Viewport[0].Pos.y = 0;
	oculusLayer.Viewport[0].Size.h = hmdEye[0].renderTarget.h;
	oculusLayer.Viewport[0].Size.w = hmdEye[0].renderTarget.w;

	oculusLayer.Viewport[1].Pos.x = 0;
	oculusLayer.Viewport[1].Pos.y = 0;
	oculusLayer.Viewport[1].Size.h = hmdEye[1].renderTarget.h;
	oculusLayer.Viewport[1].Size.w = hmdEye[1].renderTarget.w;

	globalFramebuffers.primaryFBO->Bind();

	GL_CheckErrors();

	idAngles angTemp = ang_zero;
	idVec3 headPosTemp = vec3_zero;
	idVec3 bodyPosTemp = vec3_zero;
	idVec3 absTemp = vec3_zero;
	

	HMDGetOrientation( angTemp, headPosTemp, bodyPosTemp, absTemp, false );
}


/*
==============`
iVr::HMDGetOrientation
==============
*/

void iVr::HMDGetOrientation( idAngles &hmdAngles, idVec3 &headPositionDelta, idVec3 &bodyPositionDelta, idVec3 &absolutePosition, bool resetTrackingOffset )
{
	static int lastFrame = -1;
	static double time = 0.0;
	static ovrPosef translationPose;
	static ovrPosef	orientationPose;
	static ovrPosef cameraPose;
	static ovrPosef lastTrackedPose = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	static int currentlyTracked;
	static int lastFrameReturned = -1;
	static float lastRoll = 0.0f;
	static float lastPitch = 0.0f;
	static float lastYaw = 0.0f;
	static idVec3 lastHmdPosition = vec3_zero;

	static idVec3 hmdPosition;
	static idVec3 lastHmdPos2 = vec3_zero;
	static idMat3 hmdAxis = mat3_identity;

	static bool	neckInitialized = false;
	static idVec3 initialNeckPosition = vec3_zero;
	static idVec3 currentNeckPosition = vec3_zero;
	static idVec3 lastNeckPosition = vec3_zero;

	static idVec3 currentChestPosition = vec3_zero;
	static idVec3 lastChestPosition = vec3_zero;

	static float chestLength = 0;
	static bool chestInitialized = false;

	idVec3 neckToChestVec = vec3_zero;
	idMat3 neckToChestMat = mat3_identity;
	idAngles neckToChestAng = ang_zero;

	static idVec3 lastHeadPositionDelta = vec3_zero;
	static idVec3 lastBodyPositionDelta = vec3_zero;
	static idVec3 lastAbsolutePosition = vec3_zero;
	
	if ( !hasOculusRift || !hasHMD )
	{
		hmdAngles.roll = 0.0f;
		hmdAngles.pitch = 0.0f;
		hmdAngles.yaw = 0.0f;
		headPositionDelta = vec3_zero;
		bodyPositionDelta = vec3_zero;
		absolutePosition = vec3_zero;
		return;
	}


	lastBodyYawOffset = bodyYawOffset;
	poseLastHmdAngles = poseHmdAngles;
	poseLastHmdHeadPositionDelta = poseHmdHeadPositionDelta;
	poseLastHmdBodyPositionDelta = poseHmdBodyPositionDelta;
	poseLastHmdAbsolutePosition = poseHmdAbsolutePosition;
	
	
	if ( vr_frameCheck.GetInteger() == 1 && idLib::frameNumber == lastFrame ) //&& !commonVr->renderingSplash )
	{
		//make sure to return the same values for this frame.
		hmdAngles.roll = lastRoll;
		hmdAngles.pitch = lastPitch;
		hmdAngles.yaw = lastYaw;
		headPositionDelta = lastHeadPositionDelta;
		bodyPositionDelta = lastBodyPositionDelta;

		if ( resetTrackingOffset == true )
		{

			trackingOriginOffset = lastHmdPosition;
			common->Printf( "Resetting tracking yaw offset.\n Yaw = %f old offset = %f ", hmdAngles.yaw, trackingOriginYawOffset );
			trackingOriginYawOffset = hmdAngles.yaw;
			common->Printf( "New Tracking yaw offset %f\n", hmdAngles.yaw, trackingOriginYawOffset );
			neckInitialized = false;

			
		}
		//common->Printf( "Bail lastframe == idLib:: framenumber  lf %d  ilfn %d  rendersplash = %d\n", lastFrame, idLib::frameNumber, commonVr->renderingSplash );
		return;
	}

	lastFrame = idLib::frameNumber;
		
	hmdFrameTime = ovr_GetPredictedDisplayTime( hmdSession, lastFrame ); // renderSystem->GetFrameCount() );// renderSystem->GetFrameCount() );
	time = hmdFrameTime;// .DisplayMidpointSeconds;
	//common->Printf( "Frame Getting predicted time for frame %d\n", lastFrame );

	hmdTrackingState = ovr_GetTrackingState( hmdSession, time, false );

	currentlyTracked =  hmdTrackingState.StatusFlags & ( ovrStatus_PositionTracked );
	
	if ( currentlyTracked )
	{
		translationPose = hmdTrackingState.HeadPose.ThePose;
		lastTrackedPose = translationPose;
	}
	else
	{
		translationPose = lastTrackedPose;
	}
		
	commonVr->handPose[1] = hmdTrackingState.HandPoses[ovrHand_Left].ThePose;
	commonVr->handPose[0] = hmdTrackingState.HandPoses[ovrHand_Right].ThePose;

	for ( int i = 0; i < 2; i++ )
	{
		MotionControlGetHand( i, poseHandPos[i], poseHandRotationQuat[i] );
		poseHandRotationMat3[i] = poseHandRotationQuat[i].ToMat3();
		poseHandRotationAngles[i] = poseHandRotationQuat[i].ToAngles();
	}
		
	static idAngles poseAngles = ang_zero;

	static float x, y, z = 0.0f;

	hmdPosition.x = -translationPose.Position.z * 39.3701f; // koz convert position (in meters) to inch (1 id unit = 1 inch).   
	hmdPosition.y = -translationPose.Position.x * 39.3701f;
	hmdPosition.z = translationPose.Position.y * 39.3701f;

	lastHmdPosition = hmdPosition;

		
	orientationPose = hmdTrackingState.HeadPose.ThePose;
	//cameraPose = hmdTrackingState.HeadPose. CameraPose;


	static idQuat poseRot;// = idQuat_zero;
	static idAngles poseAng = ang_zero;

	poseRot.x = orientationPose.Orientation.z;	// x;
	poseRot.y = orientationPose.Orientation.x;	// y;
	poseRot.z = -orientationPose.Orientation.y;	// z;
	poseRot.w = orientationPose.Orientation.w;

	poseAng = poseRot.ToAngles();

	hmdAngles.roll = poseAng.roll;
	hmdAngles.pitch = poseAng.pitch;
	hmdAngles.yaw = poseAng.yaw;

	lastRoll = hmdAngles.roll;
	lastPitch = hmdAngles.pitch;
	lastYaw = hmdAngles.yaw;

	hmdPosition += hmdForwardOffset * poseAngles.ToMat3()[0];

	if ( resetTrackingOffset == true )
	{

		trackingOriginOffset = lastHmdPosition;
		common->Printf( "Resetting tracking yaw offset.\n Yaw = %f old offset = %f ", hmdAngles.yaw, trackingOriginYawOffset );
		trackingOriginYawOffset = hmdAngles.yaw;
		common->Printf( "New Tracking yaw offset %f\n", hmdAngles.yaw, trackingOriginYawOffset );
		neckInitialized = false;

		return;
	}

	hmdPosition -= trackingOriginOffset;

	hmdPosition *= idAngles( 0.0f, -trackingOriginYawOffset, 0.0f ).ToMat3();

	absolutePosition = hmdPosition;

	hmdAngles.yaw -= trackingOriginYawOffset;
	hmdAngles.Normalize360();

	//	common->Printf( "Hmdangles yaw = %f pitch = %f roll = %f\n", poseAngles.yaw, poseAngles.pitch, poseAngles.roll );
	//	common->Printf( "Trans x = %f y = %f z = %f\n", hmdPosition.x, hmdPosition.y, hmdPosition.z );

	lastRoll = hmdAngles.roll;
	lastPitch = hmdAngles.pitch;
	lastYaw = hmdAngles.yaw;
	lastAbsolutePosition = absolutePosition;
	hmdPositionTracked = true;

	commonVr->hmdBodyTranslation = absolutePosition;

	idAngles hmd2 = hmdAngles;
	hmd2.yaw -= commonVr->bodyYawOffset;

	//hmdAxis = hmd2.ToMat3();
	hmdAxis = hmdAngles.ToMat3();

	currentNeckPosition = hmdPosition + hmdAxis[0] * vr_nodalX.GetFloat() /*+ hmdAxis[1] * 0.0f */ + hmdAxis[2] * vr_nodalZ.GetFloat();
		
//	currentNeckPosition.z = pm_normalviewheight.GetFloat() - (vr_nodalZ.GetFloat() + currentNeckPosition.z);

	/*
	if ( !chestInitialized )
	{
		if ( chestDefaultDefined )
		{
				
			neckToChestVec = currentNeckPosition - gameLocal.GetLocalPlayer()->chestPivotDefaultPos;
			chestLength = neckToChestVec.Length();
			chestInitialized = true;
			common->Printf( "Chest Initialized, length %f\n", chestLength );
			common->Printf( "Chest default position = %s\n", gameLocal.GetLocalPlayer()->chestPivotDefaultPos.ToString() );
		}
	}

	if ( chestInitialized )
	{
		neckToChestVec = currentNeckPosition - gameLocal.GetLocalPlayer()->chestPivotDefaultPos;
		neckToChestVec.Normalize();

		idVec3 chesMove = chestLength * neckToChestVec;
		currentChestPosition = currentNeckPosition - chesMove;

		common->Printf( "Chest length %f angles roll %f pitch %f yaw %f \n", chestLength, neckToChestVec.ToAngles().roll, neckToChestVec.ToAngles().pitch, neckToChestVec.ToAngles().yaw );
		common->Printf( "CurrentNeckPos = %s\n", currentNeckPosition.ToString() );
		common->Printf( "CurrentChestPos = %s\n", currentChestPosition.ToString() );
		common->Printf( "ChestMove = %s\n", chesMove.ToString() );

		idAngles chestAngles = ang_zero;
		chestAngles.roll = neckToChestVec.ToAngles().yaw + 90.0f;
		chestAngles.pitch = 0;// neckToChestVec.ToAngles().yaw;            //chest angles.pitch rotates the chest.
		chestAngles.yaw = 0;


		//lastView = commonVr->lastHMDViewAxis.ToAngles();
		//headAngles.roll = lastView.pitch;
		//headAngles.pitch = commonVr->lastHMDYaw - commonVr->bodyYawOffset;
		//headAngles.yaw = lastView.roll;
		//headAngles.Normalize360();
		//gameLocal.GetLocalPlayer()->GetAnimator()->SetJointAxis( gameLocal.GetLocalPlayer()->chestPivotJoint, JOINTMOD_LOCAL, chestAngles.ToMat3() );
	}
	*/
	if ( !neckInitialized )
	{
		lastNeckPosition = currentNeckPosition;
		initialNeckPosition = currentNeckPosition;
		neckInitialized = true;
	}
	
	bodyPositionDelta = currentNeckPosition - lastNeckPosition; // use this to base movement on neck model
	bodyPositionDelta.z = currentNeckPosition.z - initialNeckPosition.z;

	//bodyPositionDelta = currentChestPosition - lastChestPosition;
	lastBodyPositionDelta = bodyPositionDelta;
		
	lastNeckPosition = currentNeckPosition;
	lastChestPosition = currentChestPosition;
		
	headPositionDelta = hmdPosition - currentNeckPosition; // use this to base movement on neck model
	//headPositionDelta = hmdPosition - currentChestPosition;
	headPositionDelta.z = hmdPosition.z;
	
	//bodyPositionDelta.z = 0;

	lastBodyPositionDelta = bodyPositionDelta;
	lastHeadPositionDelta = headPositionDelta;
	
	
	/*
	else
	{
		common->Printf( "Pose invalid!!\n" );

		headPositionDelta = lastHeadPositionDelta;
		bodyPositionDelta = lastBodyPositionDelta;
		absolutePosition = lastAbsolutePosition;
		hmdAngles.roll = lastRoll;
		hmdAngles.pitch = lastPitch;
		hmdAngles.yaw = lastYaw;
	}
	*/
}

/*
==============
iVr::HMDGetOrientation
==============
*/

void iVr::HMDGetOrientationAbsolute( idAngles &hmdAngles, idVec3 &position )
{

}

/*
==============
iVr::HMDResetTrackingOriginOffset
==============
*/
void iVr::HMDResetTrackingOriginOffset( void )
{
	static idVec3 body = vec3_zero;
	static idVec3 head = vec3_zero;
	static idVec3 absPos = vec3_zero;
	static idAngles rot = ang_zero;

	common->Printf( "HMDResetTrackingOriginOffset called\n " );

	HMDGetOrientation( rot, head, body, absPos, true );

	common->Printf( "New Yaw offset = %f\n", commonVr->trackingOriginYawOffset );
}


/*
==============
iVr::MotionControllSetRotationOffset;
==============
*/
void iVr::MotionControlSetRotationOffset()
{

	/*
	switch ( motionControlType )
	{

	
	case  MOTION_HYDRA:
	{
		HydraSetRotationOffset();
		break;
	}
	

	default:
		break;
	}
	*/
}

/*
==============
iVr::MotionControllSetOffset;
==============
*/
void iVr::MotionControlSetOffset()
{
	/*
	switch ( motionControlType )
	{

		
		case  MOTION_HYDRA:
	
			HydraSetOffset();
			break;
	
		default:
			break;
	}
	*/
	return;
}



/*
==============
iVr::MotionControlGetOpenVrController
==============

void iVr::MotionControlGetOpenVrController( vr::TrackedDeviceIndex_t deviceNum, idVec3 &motionPosition, idQuat &motionRotation )
{

	idMat4 m_rmat4DevicePose = ConvertSteamVRMatrixToidMat4( m_rTrackedDevicePose[(int)deviceNum].mDeviceToAbsoluteTracking );
	static idQuat orientationPose;
	static idQuat poseRot;
	static idAngles poseAngles = ang_zero;
	static idAngles angTemp = ang_zero;

	motionPosition.x = -m_rmat4DevicePose[3][2] * 39.3701;
	motionPosition.y = -m_rmat4DevicePose[3][0] * 39.3701; // meters to inches
	motionPosition.z = m_rmat4DevicePose[3][1] * 39.3701;

	motionPosition -= trackingOriginOffset;
	motionPosition *= idAngles( 0.0f, (-trackingOriginYawOffset ) , 0.0f ).ToMat3();// .Inverse();

	orientationPose = m_rmat4DevicePose.ToMat3().ToQuat();

	poseRot.x = orientationPose.z;
	poseRot.y = orientationPose.x;
	poseRot.z = -orientationPose.y;
	poseRot.w = orientationPose.w;

	poseAngles = poseRot.ToAngles();

	angTemp.yaw = poseAngles.yaw;
	angTemp.roll = poseAngles.roll;
	angTemp.pitch = poseAngles.pitch;

	motionPosition -= commonVr->hmdBodyTranslation;

	angTemp.yaw -= trackingOriginYawOffset;// + bodyYawOffset;
	angTemp.Normalize360();

	motionRotation = angTemp.ToQuat();
}
*/

void iVr::MotionControlGetTouchController( int hand, idVec3 &motionPosition, idQuat &motionRotation )
{
	
	static idQuat poseRot;
	static idAngles poseAngles = ang_zero;
	static idAngles angTemp = ang_zero;

	motionPosition.x = -handPose[hand].Position.z * 39.3701f;// koz convert position (in meters) to inch (1 id unit = 1 inch).   
	
	motionPosition.y = -handPose[hand].Position.x * 39.3701f;
	
	motionPosition.z = handPose[hand].Position.y * 39.3701f;
			
	motionPosition -= trackingOriginOffset;
	
	motionPosition *= idAngles( 0.0f, (-trackingOriginYawOffset), 0.0f ).ToMat3();

	poseRot.x = handPose[hand].Orientation.z;	// x;
	poseRot.y = handPose[hand].Orientation.x;	// y;
	poseRot.z = -handPose[hand].Orientation.y;	// z;
	poseRot.w = handPose[hand].Orientation.w;
	
	poseAngles = poseRot.ToAngles();

	angTemp.yaw = poseAngles.yaw;
	angTemp.roll = poseAngles.roll;
	angTemp.pitch = poseAngles.pitch;

	motionPosition -= commonVr->hmdBodyTranslation;

	angTemp.yaw -= trackingOriginYawOffset;// + bodyYawOffset;
	angTemp.Normalize360();

	motionRotation = angTemp.ToQuat();
}
/*
==============
iVr::MotionControllGetHand;
==============
*/
void iVr::MotionControlGetHand( int hand, idVec3 &motionPosition, idQuat &motionRotation )
{
	if ( hand == HAND_LEFT )
	{
		MotionControlGetLeftHand( motionPosition, motionRotation );
	}
	else
	{
		MotionControlGetRightHand( motionPosition, motionRotation );
	}

	// apply weapon mount offsets
	
	if ( hand == vr_weaponHand.GetInteger() && vr_mountedWeaponController.GetBool() )
	{
		idVec3 controlToHand = idVec3( vr_mountx.GetFloat(), vr_mounty.GetFloat(), vr_mountz.GetFloat() );
		idVec3 controlCenter = idVec3( vr_vcx.GetFloat(), vr_vcy.GetFloat(), vr_vcz.GetFloat() );

		motionPosition += ( controlToHand - controlCenter ) * motionRotation; // pivot around the new point
	}
	else
	{
		motionPosition += idVec3( vr_vcx.GetFloat(), vr_vcy.GetFloat(), vr_vcz.GetFloat() ) * motionRotation;
	}
}


/*
==============
iVr::MotionControllGetLeftHand;
==============
*/
void iVr::MotionControlGetLeftHand( idVec3 &motionPosition, idQuat &motionRotation )
{
	static idAngles angles = ang_zero;
	switch ( motionControlType )
	{
	
		/*
		case MOTION_STEAMVR:
		{
			//vr::TrackedDeviceIndex_t deviceNo = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_LeftHand );
			//MotionControlGetOpenVrController( deviceNo, motionPosition, motionRotation );
			MotionControlGetOpenVrController( leftControllerDeviceNo, motionPosition, motionRotation );

			//motionPosition += idVec3( vr_vcx.GetFloat(), vr_vcy.GetFloat(), vr_vcz.GetFloat() ) * motionRotation;

			break;
		}
		*/
		case MOTION_OCULUS:
		{
		
			MotionControlGetTouchController( 1, motionPosition, motionRotation );
			break;
		}
	default:
		break;
	}
}

/*
==============
iVr::MotionControllGetRightHand;
==============
*/
void iVr::MotionControlGetRightHand( idVec3 &motionPosition, idQuat &motionRotation )
{
	static idAngles angles = ang_zero;
	switch ( motionControlType )
	{
		/*
		case MOTION_STEAMVR:
		{
			//vr::TrackedDeviceIndex_t deviceNo = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_RightHand );
			//MotionControlGetOpenVrController( deviceNo, motionPosition, motionRotation );
			MotionControlGetOpenVrController( rightControllerDeviceNo, motionPosition, motionRotation );

			//motionPosition += idVec3( vr_vcx.GetFloat(), vr_vcy.GetFloat(), vr_vcz.GetFloat() ) * motionRotation;
			break;
		}
		*/
		case MOTION_OCULUS:
		{

			MotionControlGetTouchController( 0, motionPosition, motionRotation );
			break;
		}
		default:
			break;
	}
}

/*
==============
iVr::MotionControllSetHaptic
==============
*/
void iVr::MotionControllerSetHaptic( float low, float hi )
{
	
	float beat;
	float enable;
	
	beat = fabs( low - hi ) / 65535;
	
	enable = ( beat > 0.0f) ? 1.0f : 0.0f;
		
	if ( vr_weaponHand.GetInteger() == HAND_RIGHT )
	{
		ovr_SetControllerVibration( hmdSession, ovrControllerType_RTouch, beat, enable );
	}
	else
	{
		ovr_SetControllerVibration( hmdSession, ovrControllerType_LTouch, beat, enable );
	}

	return;
}

/*
==============
iVr::CalcAimMove
Pass the controller yaw & pitch changes.
Indepent weapon view angles will be updated,
and the correct yaw & pitch movement values will
be returned based on the current user aim mode.
==============
*/

void iVr::CalcAimMove( float &yawDelta, float &pitchDelta )
{

	if ( commonVr->VR_USE_MOTION_CONTROLS ) // no independent aim or joystick pitch when using motion controllers.
	{
		pitchDelta = 0.0f;
		return;
	}

	float pitchDeadzone = vr_deadzonePitch.GetFloat();
	float yawDeadzone = vr_deadzoneYaw.GetFloat();


	commonVr->independentWeaponPitch += pitchDelta;
	commonVr->independentWeaponYaw += yawDelta;


	if ( commonVr->independentWeaponPitch >= pitchDeadzone ) commonVr->independentWeaponPitch = pitchDeadzone;
	if ( commonVr->independentWeaponPitch < -pitchDeadzone ) commonVr->independentWeaponPitch = -pitchDeadzone;
	pitchDelta = 0;

	if ( commonVr->independentWeaponYaw >= yawDeadzone )
	{
		yawDelta = commonVr->independentWeaponYaw - yawDeadzone;
		commonVr->independentWeaponYaw = yawDeadzone;
		return;
	}

	if ( commonVr->independentWeaponYaw < -yawDeadzone )
	{
		yawDelta = commonVr->independentWeaponYaw + yawDeadzone;
		commonVr->independentWeaponYaw = -yawDeadzone;
		return;
	}

	yawDelta = 0.0f;

}



/*
==============
iVr::FrameStart
==============
*/
void iVr::FrameStart( void )
{
	
	HMDGetOrientation( poseHmdAngles, poseHmdHeadPositionDelta, poseHmdBodyPositionDelta, poseHmdAbsolutePosition, false );
	return;

	static int lastFrame = -1;

	if ( idLib::frameNumber == lastFrame && !commonVr->renderingSplash ) return;
	lastFrame = idLib::frameNumber;

	lastBodyYawOffset = bodyYawOffset;
	poseLastHmdAngles = poseHmdAngles;
	poseLastHmdHeadPositionDelta = poseHmdHeadPositionDelta;
	poseLastHmdBodyPositionDelta = poseHmdBodyPositionDelta;
	poseLastHmdAbsolutePosition = poseHmdAbsolutePosition;
	
	

	HMDGetOrientation( poseHmdAngles, poseHmdHeadPositionDelta, poseHmdBodyPositionDelta, poseHmdAbsolutePosition,  false );
	
	commonVr->handPose[1] = hmdTrackingState.HandPoses[ovrHand_Left].ThePose;
	commonVr->handPose[0] = hmdTrackingState.HandPoses[ovrHand_Right].ThePose;

	for ( int i = 0; i < 2; i++ )
	{
		MotionControlGetHand( i, poseHandPos[i], poseHandRotationQuat[i] );
		poseHandRotationMat3[i] = poseHandRotationQuat[i].ToMat3();
		poseHandRotationAngles[i] = poseHandRotationQuat[i].ToAngles();
	}
	
	return;
}

/*
==============
iVr::GetCurrentFlashMode();
==============
*/

int iVr::GetCurrentFlashMode()
{
	//common->Printf( "Returning flashmode %d\n", currentFlashMode );
	return currentFlashMode;
}

/*
==============
iVr::GetCurrentFlashMode();
==============
*/
void iVr::NextFlashMode()
{
	currentFlashMode++;
	if ( currentFlashMode >= FLASH_MAX ) currentFlashMode = 0;
}

