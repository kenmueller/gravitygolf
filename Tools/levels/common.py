"""Shared helpers for level-generation scripts. Levels are build output; these scripts are the source."""

import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ASSETS = unreal.EditorAssetLibrary

CUBE = "/Engine/BasicShapes/Cube.Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"
PLANE = "/Engine/BasicShapes/Plane.Plane"
SPHERE = "/Engine/BasicShapes/Sphere.Sphere"


def fresh_level(path):
    """Open the level empty so the script fully defines it."""
    discard = unreal.LevelEditorUnsavedChangesPolicy.DISCARD
    if ASSETS.does_asset_exist(path):
        LES.load_level(path, unsaved_changes_policy=discard)
        for actor in EAS.get_all_level_actors():
            EAS.destroy_actor(actor)
    else:
        LES.load_level("/Engine/Maps/Entry", unsaved_changes_policy=discard)
        if not LES.new_level(path, False):
            raise RuntimeError(f"could not create {path}")


def scan_prefix(folder):
    """Texture path prefix of a Megascans surface installed from Fab, e.g. '/Game/Fab/X/id_tier_1/Textures/T_id_4K_'."""
    for asset in ASSETS.list_assets(f"/Game/Fab/{folder}", recursive=True):
        package = asset.split(".")[0]
        if "/Textures/" in package and package.endswith("_B"):
            return package[:-1]
    raise RuntimeError(f"no scanned textures under /Game/Fab/{folder}")


def fab_mesh(folder):
    """The static mesh of a Megascans 3D asset installed from Fab."""
    for asset in ASSETS.list_assets(f"/Game/Fab/{folder}", recursive=True):
        package = asset.split(".")[0]
        if "/StaticMeshes/" in package:
            mesh = unreal.load_asset(package)
            nanite = mesh.get_editor_property("nanite_settings")
            if not nanite.enabled:
                nanite.enabled = True
                mesh.set_editor_property("nanite_settings", nanite)
                ASSETS.save_loaded_asset(mesh)
            return mesh
    raise RuntimeError(f"no static mesh under /Game/Fab/{folder}")


def scan_material(name, folder, tile, tint=(1, 1, 1, 1), roughness=1.0, stripes=(0.0, 0.0)):
    """A world-aligned material from a Megascans surface: seamless on any box, cliff or drift."""
    instance = material_instance("/Game/Materials/M_GG_Scan.M_GG_Scan", name,
                                 {"Tile": tile, "RoughnessScale": roughness, "StripeWidth": stripes[0],
                                  "StripeStrength": stripes[1]}, {"Tint": tint})
    prefix = scan_prefix(folder)
    mel = unreal.MaterialEditingLibrary
    for param, suffix in (("BaseTex", "B"), ("NormalTex", "N"), ("OrmTex", "ORM")):
        mel.set_material_instance_texture_parameter_value(instance, param, unreal.load_asset(prefix + suffix))
    ASSETS.save_loaded_asset(instance)
    return instance


def static_mesh(mesh, location, rotation=(0, 0, 0), scale=(1, 1, 1), material=None, label=None, folder=None):
    actor = spawn(unreal.StaticMeshActor, location, rotation, label, folder)
    actor.static_mesh_component.set_static_mesh(mesh)
    if material:
        actor.static_mesh_component.set_material(0, material)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


def rotator(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def spawn(cls, location=(0, 0, 0), rotation=(0, 0, 0), label=None, folder=None):
    actor = EAS.spawn_actor_from_class(cls, unreal.Vector(*location), rotator(*rotation))
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    return actor


def mesh(asset, location, size, rotation=(0, 0, 0), material=None, label=None, folder=None, cast_shadow=True,
         solid=True):
    """A static mesh actor. Size is in centimeters for the engine's 100 cm basic shapes. Pass solid=False for
    scenery the player and traces should pass through (sky domes, far props, floating lights)."""
    actor = spawn(unreal.StaticMeshActor, location, rotation, label, folder)
    component = actor.static_mesh_component
    if not solid:
        component.set_collision_profile_name("NoCollision")
    component.set_static_mesh(unreal.load_asset(asset))
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    if material:
        component.set_material(0, material)
    component.set_editor_property("cast_shadow", cast_shadow)
    return actor


def box(center, size, material, label=None, folder="Course", rotation=(0, 0, 0), solid=True):
    return mesh(CUBE, center, size, rotation, material, label, folder, solid=solid)


def material_instance(parent_path, name, scalars=None, vectors=None, folder="/Game/Materials/Instances"):
    """Create (or recreate) a material instance constant with parameter overrides."""
    path = f"{folder}/{name}"
    if ASSETS.does_asset_exist(path):
        ASSETS.delete_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    instance = tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel = unreal.MaterialEditingLibrary
    mel.set_material_instance_parent(instance, unreal.load_asset(parent_path))
    for key, value in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(instance, key, value)
    for key, value in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(instance, key, unreal.LinearColor(*value))
    ASSETS.save_asset(path)
    return instance


def player_start(course):
    """Where the player appears: just behind the ball at the tee, facing down the hole."""
    tee = course["tee"]["position"]
    yaw = course["tee"]["yaw"]
    spawn(unreal.PlayerStart, (tee[0] - 170, tee[1] - 45, tee[2] + 110), (0, yaw, 0), "PlayerStart", "Course")


def night_sky(moon_pitch=-32.0, moon_yaw=40.0, moon_lux=0.35, fog_density=0.01, exposure_bias=-1.2):
    """Night: a star dome, a moon that lights the scene, a sky light fed by the dome, thin fog."""
    sky = unreal.load_asset("/Game/Materials/M_GG_NightSky.M_GG_NightSky")
    mesh(SPHERE, (0, 0, 0), (80000000, 80000000, 80000000), material=sky, label="NightSky", folder="Lighting",
         cast_shadow=False, solid=False)

    moon = spawn(unreal.DirectionalLight, (0, 0, 5000), (moon_pitch, moon_yaw, 0), "Moon", "Lighting")
    light = moon.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_intensity(moon_lux)
    light.set_light_color(unreal.LinearColor(0.62, 0.74, 1.0, 1.0))
    light.set_editor_property("light_source_angle", 0.8)

    fog = spawn(unreal.ExponentialHeightFog, (0, 0, -500), label="Fog", folder="Lighting")
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_component.set_fog_density(fog_density)
    fog_component.set_editor_property("fog_height_falloff", 0.08)
    fog_component.set_fog_inscattering_color(unreal.LinearColor(0.02, 0.03, 0.06, 1.0))
    fog_component.set_volumetric_fog(True)

    sky_light = spawn(unreal.SkyLight, (0, 0, 1000), label="SkyLight", folder="Lighting")
    sky_component = sky_light.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("real_time_capture", True)
    sky_component.set_intensity(2.0)

    post = spawn(unreal.PostProcessVolume, (0, 0, 0), label="Post", folder="Lighting")
    post.set_editor_property("unbound", True)
    settings = post.settings
    settings.set_editor_property("override_auto_exposure_method", True)
    settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    settings.set_editor_property("override_auto_exposure_bias", True)
    settings.set_editor_property("auto_exposure_bias", exposure_bias)
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 0.8)
    settings.set_editor_property("override_lens_flare_intensity", True)
    settings.set_editor_property("lens_flare_intensity", 0.0)
    settings.set_editor_property("override_vignette_intensity", True)
    settings.set_editor_property("vignette_intensity", 0.4)
    post.set_editor_property("settings", settings)
    return moon


def point_light(location, color, candelas, radius, label, folder="Lighting/Lanterns"):
    actor = spawn(unreal.PointLight, location, label=label, folder=folder)
    light = actor.get_component_by_class(unreal.PointLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    light.set_intensity(candelas)
    light.set_light_color(unreal.LinearColor(color[0], color[1], color[2], 1.0))
    light.set_attenuation_radius(radius)
    light.set_editor_property("source_radius", 12.0)
    return actor


def sky_and_sun(sun_pitch=-8.0, sun_yaw=135.0, sun_lux=10.0, fog_density=0.015, exposure_bias=0.0, sun_kelvin=5200.0,
                sky_intensity=1.0):
    """Physically based sky: atmosphere, volumetric clouds, height fog, real-time sky light and the sun."""
    sun = spawn(unreal.DirectionalLight, (0, 0, 5000), (sun_pitch, sun_yaw, 0), "Sun", "Lighting")
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_intensity(sun_lux)
    light.set_atmosphere_sun_light(True)
    light.set_editor_property("light_source_angle", 0.6)
    light.set_editor_property("use_temperature", True)
    light.set_editor_property("temperature", sun_kelvin)
    light.set_editor_property("forward_shading_priority", 1)
    light.set_editor_property("cast_shadows_on_atmosphere", True)
    light.set_editor_property("cast_cloud_shadows", False)

    spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere", folder="Lighting")

    clouds = spawn(unreal.VolumetricCloud, (0, 0, 0), label="Clouds", folder="Lighting")
    cloud_component = clouds.get_component_by_class(unreal.VolumetricCloudComponent)
    cloud_component.set_editor_property("layer_bottom_altitude", 3.0)
    cloud_component.set_editor_property("layer_height", 6.0)
    # Measured on an M4 Max: at full sample counts the clouds cost 8 ms a frame (the view, plus the sky light's
    # capture of them). These settings cost under half that and look the same in play.
    cloud_component.set_view_sample_count_scale(0.5)
    cloud_component.set_shadow_view_sample_count_scale(0.5)
    cloud_component.set_reflection_view_sample_count_scale(0.3)
    cloud_component.set_shadow_reflection_view_sample_count_scale(0.3)

    fog = spawn(unreal.ExponentialHeightFog, (0, 0, -2500), label="Fog", folder="Lighting")
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_component.set_fog_density(fog_density)
    fog_component.set_editor_property("fog_height_falloff", 0.12)
    fog_component.set_volumetric_fog(True)
    fog_component.set_editor_property("volumetric_fog_scattering_distribution", 0.6)

    sky = spawn(unreal.SkyLight, (0, 0, 1000), label="SkyLight", folder="Lighting")
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("real_time_capture", True)
    sky_component.set_intensity(sky_intensity)

    post = spawn(unreal.PostProcessVolume, (0, 0, 0), label="Post", folder="Lighting")
    post.set_editor_property("unbound", True)
    settings = post.settings
    settings.set_editor_property("override_auto_exposure_method", True)
    settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    settings.set_editor_property("override_auto_exposure_bias", True)
    settings.set_editor_property("auto_exposure_bias", exposure_bias)
    # Local exposure keeps shadowed hedges and tree trunks readable beside sunlit stone.
    settings.set_editor_property("override_local_exposure_shadow_contrast_scale", True)
    settings.set_editor_property("local_exposure_shadow_contrast_scale", 0.72)
    settings.set_editor_property("override_local_exposure_highlight_contrast_scale", True)
    settings.set_editor_property("local_exposure_highlight_contrast_scale", 0.85)
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 0.45)
    settings.set_editor_property("override_lens_flare_intensity", True)
    settings.set_editor_property("lens_flare_intensity", 0.0)
    settings.set_editor_property("override_vignette_intensity", True)
    settings.set_editor_property("vignette_intensity", 0.28)
    settings.set_editor_property("override_film_grain_intensity", True)
    settings.set_editor_property("film_grain_intensity", 0.08)
    post.set_editor_property("settings", settings)
    return sun
