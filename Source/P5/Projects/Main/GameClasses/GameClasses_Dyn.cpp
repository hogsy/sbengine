
#include "MRTC.h"

class CRegisterGameClasses
{
public:
	CRegisterGameClasses()
	{
		// Game classes

//		MRTC_REFERENCE(CRPG_Object_CTFItem);
//		MRTC_REFERENCE(CXR_Model_AttachFlare);
//		MRTC_REFERENCE(CXR_Model_BloodSpurt);
//		MRTC_REFERENCE(CXR_Model_Butterfly);
//		MRTC_REFERENCE(CXR_Model_CataBall);
//		MRTC_REFERENCE(CXR_Model_CrossHair);
//		MRTC_REFERENCE(CXR_Model_Cylinder);
		MRTC_REFERENCE(CXR_Model_GlassSystem);
//		MRTC_REFERENCE(CXR_Model_DavidTest);
//		MRTC_REFERENCE(CXR_Model_DavidTest);
		MRTC_REFERENCE(CXR_Model_Debris);
//		MRTC_REFERENCE(CXR_Model_DebrisTest);
//		MRTC_REFERENCE(CXR_Model_RocketDebris);
//		MRTC_REFERENCE(CXR_Model_SurfaceDebris);
		MRTC_REFERENCE(CXR_Model_Impact);
		MRTC_REFERENCE(CXR_Model_ExplosionDebris);
		MRTC_REFERENCE(CXR_Model_RocketTrail);
//		MRTC_REFERENCE(CXR_Model_Disc);
//		MRTC_REFERENCE(CXR_Model_DustCloud);
//		MRTC_REFERENCE(CXR_Model_DynLight);
//		MRTC_REFERENCE(CXR_Model_Electric);
//		MRTC_REFERENCE(CXR_Model_ExpandRing);
//		MRTC_REFERENCE(CXR_Model_ExpandSphere);
//		MRTC_REFERENCE(CXR_Model_Explosion);
//		MRTC_REFERENCE(CXR_Model_FireBlade);
		MRTC_REFERENCE(CXR_Model_FocusFrame);
//		MRTC_REFERENCE(CXR_Model_Fuse);
//		MRTC_REFERENCE(CXR_Model_FuseSparks);
//		MRTC_REFERENCE(CXR_Model_Glimmer);
//		MRTC_REFERENCE(CXR_Model_Glimmer2);
//		MRTC_REFERENCE(CXR_Model_InstantTrail);
		MRTC_REFERENCE(CXR_Model_EffectSystem);
		MRTC_REFERENCE(CXR_Model_BloodEffect);
		MRTC_REFERENCE(CXR_Model_Drain);
		MRTC_REFERENCE(CXR_Model_InstantTrail2);
		MRTC_REFERENCE(CXR_Model_TrailStrip);
		MRTC_REFERENCE(CXR_Model_MuzzleFlame);
		MRTC_REFERENCE(CXR_Model_MuzzleRiot);
		MRTC_REFERENCE(CXR_Model_MuzzleRiotSingle);
		MRTC_REFERENCE(CXR_Model_MuzzleAttackDroid);
		MRTC_REFERENCE(CXR_Model_MuzzleShotgun);
		MRTC_REFERENCE(CXR_Model_None);
		MRTC_REFERENCE(CXR_Model_LaserBeam);
//		MRTC_REFERENCE(CXR_Model_LeafDripper);
//		MRTC_REFERENCE(CXR_Model_ParticlesOpt);
		MRTC_REFERENCE(CXR_Model_Particles);
		MRTC_REFERENCE(CXR_Model_PhysicsPrim);
		MRTC_REFERENCE(CXR_Model_Line);
//		MRTC_REFERENCE(CXR_Model_ShockWave);
//		MRTC_REFERENCE(CXR_Model_SpeedTrail);
//		MRTC_REFERENCE(CXR_Model_Sphere);
		MRTC_REFERENCE(CXR_Model_Sprite2);
//		MRTC_REFERENCE(CXR_Model_TeleportSuck);
///////		MRTC_REFERENCE(CXR_Model_Tentacles);
//		MRTC_REFERENCE(CXR_Model_VolumeFire);
		MRTC_REFERENCE(CXR_Model_ExplosionSystem);
		
		MRTC_REFERENCE(CXR_Model_Chain);

		//MRTC_REFERENCE(CXR_Model_RainDrops);

//		MRTC_REFERENCE(CXR_Model_Water);
//		MRTC_REFERENCE(CXR_Model_WirePath);
//		MRTC_REFERENCE(CXR_Model_Smoke);
//		MRTC_REFERENCE(CXR_Model_Flames);
//		MRTC_REFERENCE(CXR_Model_Fire);
//		MRTC_REFERENCE(CXR_Model_FireTorch);
//		MRTC_REFERENCE(CXR_Model_TraceLight2);
//		MRTC_REFERENCE(CXR_Model_BoltTrail);
//		MRTC_REFERENCE(CXR_Model_Trail);
//		MRTC_REFERENCE(CXR_Model_DoubleTrail);
//		MRTC_REFERENCE(CXR_Model_MagicTrail);
//		MRTC_REFERENCE(CXR_Model_Blizzard);
//		MRTC_REFERENCE(CXR_Model_Rail);
//		MRTC_REFERENCE(CXR_Model_FlameTounge);
//		MRTC_REFERENCE(CXR_Model_IceTounge);
//		MRTC_REFERENCE(CXR_Model_FireSphere);
//		MRTC_REFERENCE(CXR_Model_LightningSphere);
//		MRTC_REFERENCE(CXR_Model_Particles_TorchFire);
//		MRTC_REFERENCE(CXR_Model_Particles_FireWall);
//		MRTC_REFERENCE(CXR_Model_Particles_SmallFire);
//		MRTC_REFERENCE(CXR_Model_Particles_InfernoFlare);
//		MRTC_REFERENCE(CXR_Model_Particles_Sparkles);
//		MRTC_REFERENCE(CXR_Model_Particles_Explosion);
//		MRTC_REFERENCE(CXR_Model_Particles_Teleporter);
//		MRTC_REFERENCE(CXR_Model_Particles_Explosion2);
//		MRTC_REFERENCE(CXR_Model_Particles_Explosion2b);
//		MRTC_REFERENCE(CXR_Model_Particles_Blood);
		MRTC_REFERENCE(CXR_Model_WaterTile);
		MRTC_REFERENCE(CXR_Model_WaterTile2);
		//MRTC_REFERENCE(CXR_Model_WaterBase);
		MRTC_REFERENCE(CRPG_Object_Ammo);
//		MRTC_REFERENCE(CRPG_Object_BurstAmmo);
//		MRTC_REFERENCE(CRPG_Object_SniperAmmo);
//		MRTC_REFERENCE(CRPG_Object_ArrowBurst);
//		MRTC_REFERENCE(CRPG_Object_Bow);
		MRTC_REFERENCE(CRPG_Object_Char);
//		MRTC_REFERENCE(CRPG_Object_Char2);
		MRTC_REFERENCE(CRPG_Object);
		MRTC_REFERENCE(CRPG_Object_CreatureAttack);
//		MRTC_REFERENCE(CRPG_Object_DragonBreath);
//		MRTC_REFERENCE(CRPG_Object_KnuckleDuster);
		MRTC_REFERENCE(CRPG_Object_Melee);
		MRTC_REFERENCE(CRPG_Object_Gun);
		MRTC_REFERENCE(CRPG_Object_AngelusChain);
		MRTC_REFERENCE(CRPG_Object_Inventory);
//		MRTC_REFERENCE(CRPG_Object_Inventory2);
		MRTC_REFERENCE(CRPG_Object_Item);
		MRTC_REFERENCE(CRPG_Object_Collectible);
//		MRTC_REFERENCE(CRPG_Object_Item2);
		MRTC_REFERENCE(CRPG_Object_MiniGun);
		MRTC_REFERENCE(CRPG_Object_Drill);
		MRTC_REFERENCE(CRPG_Object_Pickup);
//		MRTC_REFERENCE(CRPG_Object_Potion);
//		MRTC_REFERENCE(CRPG_Object_Diversion);
//		MRTC_REFERENCE(CRPG_Object_SurveillanceMonitor);
		MRTC_REFERENCE(CRPG_Object_Rifle);
		MRTC_REFERENCE(CRPG_Object_Ancient);
		MRTC_REFERENCE(CRPG_Object_ShotGun);
		MRTC_REFERENCE(CRPG_Object_ShotGun_Pit);
//		MRTC_REFERENCE(CRPG_Object_SmartRocketLauncher);
//		MRTC_REFERENCE(CRPG_Object_RaySummon);
//		MRTC_REFERENCE(CRPG_Object_MultiRaySummon);
//		MRTC_REFERENCE(CRPG_Object_SphereDamage);
		MRTC_REFERENCE(CRPG_Object_Summon);
//		MRTC_REFERENCE(CRPG_Object_ThresholdSummon);
//		MRTC_REFERENCE(CRPG_Object_ReleaseSummon);
//		MRTC_REFERENCE(CRPG_Object_TargetSummon);
//		MRTC_REFERENCE(CRPG_Object_FloorSummon);
//		MRTC_REFERENCE(CRPG_Object_MultiSpreadSummon);
		MRTC_REFERENCE(CRPG_Object_Throw);
		MRTC_REFERENCE(CRPG_Object_Weapon);
		MRTC_REFERENCE(CRPG_Object_PhysControl);
		MRTC_REFERENCE(CRPG_Object_Flashbang);

		// Serial

		MRTC_REFERENCE(CWObject_AnimSyncer);
		MRTC_REFERENCE(CWObject_Character);
		MRTC_REFERENCE(CWObject_CharPlayer);
		MRTC_REFERENCE(CWObject_CharPlayer_P6);
		MRTC_REFERENCE(CWObject_CharNPC);
		MRTC_REFERENCE(CWObject_CharNPC_P6);
		MRTC_REFERENCE(CWObject_CharDarkling);
		MRTC_REFERENCE(CWObject_CharAngelus);
		MRTC_REFERENCE(CWObject_CharShapeshifter);
		
//		MRTC_REFERENCE(CWObject_Character2AI);
//		MRTC_REFERENCE(CWObject_Character2Player);
		MRTC_REFERENCE(CWObject_Spectator);
		MRTC_REFERENCE(CWObject_SpectatorIntermission);
//		MRTC_REFERENCE(CWObject_RPG_Item);
//		MRTC_REFERENCE(CWObject_RPG_CTFItem);
		MRTC_REFERENCE(CWObject_Player);
		MRTC_REFERENCE(CWObject_RPG);
		MRTC_REFERENCE(CWObject_Item);
//		MRTC_REFERENCE(CWObject_AreaInfo);
//		MRTC_REFERENCE(CWObject_NavNode);
		MRTC_REFERENCE(CWObject_Team);
		MRTC_REFERENCE(CWObject_GameCore);
#ifdef M_Profile
		MRTC_REFERENCE(CWObject_GameDebug);
#endif
		MRTC_REFERENCE(CWObject_GameCampaign);
		MRTC_REFERENCE(CWObject_GameP6);
		MRTC_REFERENCE(CWObject_ActionCutscene);
		MRTC_REFERENCE(CWObject_InputEntity);
		MRTC_REFERENCE(CWObject_DarklingSpawn);
		MRTC_REFERENCE(CWObject_DarklingSpawnEffect);
		MRTC_REFERENCE(CWObject_Turret);
		MRTC_REFERENCE(CWObject_Spawner_P6);
		MRTC_REFERENCE(CWObject_Crow);
		MRTC_REFERENCE(CWObject_Rat);
		MRTC_REFERENCE(CWObject_AnimEventListener);
		MRTC_REFERENCE(CWObject_AutoFire);
		MRTC_REFERENCE(CWObject_LeverActionCutscene);
		MRTC_REFERENCE(CWObject_ValveActionCutscene);
		MRTC_REFERENCE(CWObject_ActionCutsceneRiot);
		MRTC_REFERENCE(CWObject_DCActioncutscene);
		MRTC_REFERENCE(CWObject_ActionCutscenePickup);
		MRTC_REFERENCE(CWObject_ActionCutscenePeriscope);
		MRTC_REFERENCE(CWObject_Telephone);
		MRTC_REFERENCE(CWObject_TelephoneRegistry);
		MRTC_REFERENCE(CRPG_Object_Pager);
		MRTC_REFERENCE(CWObject_DroppableItem);
		MRTC_REFERENCE(CWObject_BloodEffect);
		MRTC_REFERENCE(CWObject_Destructable);
		MRTC_REFERENCE(CWObject_Mine);
		MRTC_REFERENCE(CWObject_Explosion);
		MRTC_REFERENCE(CWObject_GibSystem);
		MRTC_REFERENCE(CWObject_ClientGib);
		MRTC_REFERENCE(CWObject_GibSystem);
		MRTC_REFERENCE(CWObject_HealthStation);
		MRTC_REFERENCE(CWObject_Ledge);
		MRTC_REFERENCE(CWObject_Model_Anim);
		MRTC_REFERENCE(CWObject_Phys_Ladder);
		MRTC_REFERENCE(CWObject_Model_Moth);
//		MRTC_REFERENCE(CWObject_Prefab);
		MRTC_REFERENCE(CWObject_RigidBody);
		MRTC_REFERENCE(CWObject_ScenePoint);
		MRTC_REFERENCE(CWObject_TentacleSystem);
		MRTC_REFERENCE(CWObject_AngelusChain);
		MRTC_REFERENCE(CWObject_Wallmark);
//		MRTC_REFERENCE(CWObject_Func_FunBox);
//		MRTC_REFERENCE(CWObject_Func_Button);
		MRTC_REFERENCE(CWObject_Cutscene);
		MRTC_REFERENCE(CWObject_MusicFader);
		MRTC_REFERENCE(CWObject_MusicTrackFader);

		MRTC_REFERENCE(CWObject_Detector_Camera);
		MRTC_REFERENCE(CWObject_Detector_Camera_SecurityLow);
		MRTC_REFERENCE(CWObject_Detector_Camera_SecurityNormal);
		MRTC_REFERENCE(CWObject_Detector_Camera_SecurityHigh);
		MRTC_REFERENCE(CWObject_Emitter);
		MRTC_REFERENCE(CWObject_Emitter_Suspendable);
		MRTC_REFERENCE(CWObject_EmitterCoordinator);
//		MRTC_REFERENCE(CWObject_Func_Float);
//		MRTC_REFERENCE(CWObject_MessageBranch);
//		MRTC_REFERENCE(CWObject_MessagePipe);
//		MRTC_REFERENCE(CWObject_ObjAnim);
		MRTC_REFERENCE(CWObject_Physical);
//		MRTC_REFERENCE(CWObject_TeleportController);
		MRTC_REFERENCE(CWObject_Trigger_Ext);
		MRTC_REFERENCE(CWObject_Trigger_SoftSpot);
		MRTC_REFERENCE(CWObject_Trigger_SneakZone);
		MRTC_REFERENCE(CWObject_Trigger_RestrictedZone);
//		MRTC_REFERENCE(CWObject_Trigger_Message);
		MRTC_REFERENCE(CWObject_Trigger_Damage);
		MRTC_REFERENCE(CWObject_Trigger_Checkpoint);
//		MRTC_REFERENCE(CWObject_Trigger_RPG);
//		MRTC_REFERENCE(CWObject_Trigger_AIDie);
		MRTC_REFERENCE(CWObject_Trigger_Lamp);
		MRTC_REFERENCE(CWObject_Trigger_Surface);
		MRTC_REFERENCE(CWObject_Trigger_Pickup);
		MRTC_REFERENCE(CWObject_Trigger_AccPad);
		MRTC_REFERENCE(CWObject_Phys_Anim);
//		MRTC_REFERENCE(CWObject_ClientDebris);
		MRTC_REFERENCE(CWObject_ProjectileCluster);
//		MRTC_REFERENCE(CWObject_SmartRocket);
		MRTC_REFERENCE(CWObject_Rocket);
		MRTC_REFERENCE(CWObject_Throwable);
//		MRTC_REFERENCE(CWObject_ClientModel);
//		MRTC_REFERENCE(CWObject_ClientModel_Ext);
		MRTC_REFERENCE(CWObject_Ext_Model);
//		MRTC_REFERENCE(CWObject_Ext_ModelEmitter);
		MRTC_REFERENCE(CWObject_Damage);
//		MRTC_REFERENCE(CWObject_PainBox);
//		MRTC_REFERENCE(CWObject_Projectile);
//		MRTC_REFERENCE(CWObject_Projectile_Tracer);
//		MRTC_REFERENCE(CWObject_Projectile_Bouncer);
//		MRTC_REFERENCE(CWObject_Projectile_Bounce);
//		MRTC_REFERENCE(CWObject_Projectile_Instant);
//		MRTC_REFERENCE(CWObject_AttachModel);
//		MRTC_REFERENCE(CWObject_Projectile_Homing);
//		MRTC_REFERENCE(CWObject_Projectile_HomingSkelAnim);
		MRTC_REFERENCE(CWObject_TranquillizerDart);
		MRTC_REFERENCE(CWObject);
		MRTC_REFERENCE(CWObject_Game);
		MRTC_REFERENCE(CWObject_Game_Settings);
		MRTC_REFERENCE(CWObject_Attach);
		MRTC_REFERENCE(CWObject_Hook);
		MRTC_REFERENCE(CWObject_Engine_Rotate);
//		MRTC_REFERENCE(CWObject_Engine_Wheel);
//		MRTC_REFERENCE(CWObject_Hook_NoRotate);
		MRTC_REFERENCE(CWObject_Hook_To_Target);
//		MRTC_REFERENCE(CWObject_Hook_To_Line);
//		MRTC_REFERENCE(CWObject_Hook_To_Circle);
		MRTC_REFERENCE(CWObject_Engine_Path);
		MRTC_REFERENCE(CWObject_TimedMessages);
		MRTC_REFERENCE(CWObject_Dynamic);
		MRTC_REFERENCE(CWObject_MergedSolid);
		MRTC_REFERENCE(CWObject_Light);
		MRTC_REFERENCE(CWObject_Light2);
		MRTC_REFERENCE(CWObject_DynamicLight);
		MRTC_REFERENCE(CWObject_DynamicLight2);
#ifdef M_Profile
		MRTC_REFERENCE(CWObject_CoordinateSystem);
		MRTC_REFERENCE(CWObject_Line);
#endif
		MRTC_REFERENCE(CWObject_RailMessagePoint);
		MRTC_REFERENCE(CWObject_RailWagon);
		MRTC_REFERENCE(CWObject_RailHandler);
		MRTC_REFERENCE(CWObject_Rail);
		MRTC_REFERENCE(CWObject_SoundVolume);
		MRTC_REFERENCE(CWObject_Info_Player_Start);
#ifdef M_Profile
		MRTC_REFERENCE(CWObject_Info_Intermission);
#endif
//		MRTC_REFERENCE(CWObject_Info_Teleport_Destination);
//		MRTC_REFERENCE(CWObject_Info_ChangeWorld_Destination);
		MRTC_REFERENCE(CWObject_Model);
//#ifdef M_Profile
		MRTC_REFERENCE(CWObject_AnimModel);
//#endif
		MRTC_REFERENCE(CWObject_Sound);
		MRTC_REFERENCE(CWObject_Static);
		MRTC_REFERENCE(CWObject_Ext_Static);
		MRTC_REFERENCE(CWObject_Null);
		MRTC_REFERENCE(CWObject_WorldSpawn);
		MRTC_REFERENCE(CWObject_WorldPlayerPhys);

		MRTC_REFERENCE(CWObject_WorldGlassSpawn);
		MRTC_REFERENCE(CWObject_Glass_Dynamic);

		MRTC_REFERENCE(CWObject_WorldSky);
		MRTC_REFERENCE(CWObject_SideScene);
		MRTC_REFERENCE(CWObject_Func_UserPortal);
//		MRTC_REFERENCE(CWObject_Func_Portal);
//		MRTC_REFERENCE(CWObject_Func_Mirror);
//		MRTC_REFERENCE(CWObject_Func_LOD);
//		MRTC_REFERENCE(CWObject_FogVolume);
		MRTC_REFERENCE(CWObject_Flare);
		MRTC_REFERENCE(CWObject_Trigger);
//		MRTC_REFERENCE(CWObject_Trigger_Teleport);
		MRTC_REFERENCE(CWObject_Trigger_ChangeWorld);
//		MRTC_REFERENCE(CWObject_System);

		MRTC_REFERENCE(CWObject_CreepingDark);
		MRTC_REFERENCE(CWObject_CreepingDarkEntity);
		MRTC_REFERENCE(CWObject_SwingDoor);
		MRTC_REFERENCE(CWObject_SwingDoorAttach);
		MRTC_REFERENCE(CWObject_Object);
		MRTC_REFERENCE(CWObject_Object_Constraint);
		MRTC_REFERENCE(CWObject_Object_Lamp);
		MRTC_REFERENCE(CWObject_LampSwitch);
		MRTC_REFERENCE(CWObject_Room);
		MRTC_REFERENCE(CWObject_Cloth);
		MRTC_REFERENCE(CWObject_Television);
		MRTC_REFERENCE(CWObject_BlackHole);
		MRTC_REFERENCE(CWObject_Follower);
		MRTC_REFERENCE(CWObject_Sync);

		//MRTC_REFERENCE(CWObject_Rain);
		MRTC_REFERENCE(CWObject_EffectSystem);

		//MRTC_REFERENCE(CWObject_Wind);
		MRTC_REFERENCE(CWObject_ChainEffect);

		MRTC_REFERENCE(CWObject_Shell);
	}
};

CRegisterGameClasses g_GameClassesDyn;

