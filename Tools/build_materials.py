"""Generates every material Gravity Golf uses. The .uasset files are build output; this script is the source.

Run headless (no editor UI):
    Tools/run_python.sh Tools/build_materials.py
"""

import unreal

MEL = unreal.MaterialEditingLibrary
ASSETS = unreal.EditorAssetLibrary
FOLDER = "/Game/Materials"

# Shared HLSL helpers, embedded in each Custom node as a struct (Custom nodes can't declare free functions).
NOISE = r"""
struct GGNoise
{
    float Hash(float3 P)
    {
        P = frac(P * 0.3183099 + 0.1);
        P *= 17.0;
        return frac(P.x * P.y * P.z * (P.x + P.y + P.z));
    }
    float Noise(float3 X)
    {
        float3 I = floor(X);
        float3 F = frac(X);
        F = F * F * (3.0 - 2.0 * F);
        return lerp(
            lerp(lerp(Hash(I + float3(0, 0, 0)), Hash(I + float3(1, 0, 0)), F.x),
                 lerp(Hash(I + float3(0, 1, 0)), Hash(I + float3(1, 1, 0)), F.x), F.y),
            lerp(lerp(Hash(I + float3(0, 0, 1)), Hash(I + float3(1, 0, 1)), F.x),
                 lerp(Hash(I + float3(0, 1, 1)), Hash(I + float3(1, 1, 1)), F.x), F.y), F.z);
    }
    float Fbm(float3 P, int Octaves)
    {
        float Sum = 0.0;
        float Amp = 0.5;
        for (int K = 0; K < Octaves; K++)
        {
            Sum += Amp * Noise(P);
            P = P * 2.03 + 17.17;
            Amp *= 0.5;
        }
        return Sum;
    }
};
GGNoise N;
"""

BALL = NOISE + r"""
// Golf ball dimples: distance to the nearest point on a jittered spherical lattice, as a height.
float3 D = normalize(LocalPos);
float3 P = D * 9.0;
float3 I = floor(P);
float Nearest = 10.0;
for (int X = -1; X <= 1; X++)
for (int Y = -1; Y <= 1; Y++)
for (int Z = -1; Z <= 1; Z++)
{
    float3 C = I + float3(X, Y, Z);
    float3 Site = C + float3(N.Hash(C), N.Hash(C + 3.1), N.Hash(C + 5.7)) * 0.7 + 0.15;
    Site = normalize(Site) * 9.0;
    Nearest = min(Nearest, distance(P, Site));
}
return saturate(Nearest * 1.6);
"""


def fresh(name):
    path = f"{FOLDER}/{name}"
    if ASSETS.does_asset_exist(path):
        ASSETS.delete_asset(path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, FOLDER, unreal.Material, unreal.MaterialFactoryNew()
    )
    return material, path


def node(material, cls, x, y, **props):
    expression = MEL.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        expression.set_editor_property(key, value)
    return expression


def vector_param(material, name, color, x, y):
    return node(material, unreal.MaterialExpressionVectorParameter, x, y,
                parameter_name=name, default_value=unreal.LinearColor(*color))


def scalar_param(material, name, value, x, y):
    return node(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)


def custom(material, code, inputs, x, y, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, outputs=()):
    expression = node(material, unreal.MaterialExpressionCustom, x, y, code=code, output_type=output_type,
                      description="GG")
    expression.set_editor_property("inputs", [struct(unreal.CustomInput, input_name=name) for name in inputs])
    if outputs:
        expression.set_editor_property(
            "additional_outputs",
            [struct(unreal.CustomOutput, output_name=name, output_type=kind) for name, kind in outputs],
        )
    return expression


def struct(cls, **props):
    # Unreal structs take no constructor arguments in Python; fields are set afterwards.
    value = cls()
    for key, item in props.items():
        value.set_editor_property(key, item)
    return value


def finish(material, path):
    MEL.recompile_material(material)
    ASSETS.save_asset(path)
    unreal.log(f"GG: built {path}")


def build_surface():
    material, path = fresh("M_GG_Surface")
    base = vector_param(material, "BaseColor", (0.8, 0.8, 0.8, 1), -600, -200)
    rough = scalar_param(material, "Roughness", 0.5, -600, 0)
    metal = scalar_param(material, "Metallic", 0.0, -600, 100)
    emissive = vector_param(material, "EmissiveColor", (0, 0, 0, 1), -600, 250)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(material, path)


def build_glow():
    material, path = fresh("M_GG_Glow")
    material.set_editor_property("used_with_instanced_static_meshes", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = vector_param(material, "Color", (1, 1, 1, 1), -600, 0)
    strength = scalar_param(material, "Strength", 10.0, -600, 200)
    product = node(material, unreal.MaterialExpressionMultiply, -300, 50)
    MEL.connect_material_expressions(color, "", product, "A")
    MEL.connect_material_expressions(strength, "", product, "B")
    MEL.connect_material_property(product, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(material, path)


LANTERN = r"""
// A paper lantern lit from inside: brightest around the flame a little below the middle, dimmer toward the top
// and bottom, with faint ribs. LocalPos is in the cube's own space (-50..50).
float H = LocalPos.z / 50.0;
float Flame = exp(-pow((H + 0.22) * 1.7, 2.0));
float Ribs = 0.88 + 0.12 * cos(LocalPos.z * 0.63);
return (0.16 + 0.84 * Flame) * Ribs;
"""


def build_lantern():
    material, path = fresh("M_GG_Lantern")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    local = node(material, unreal.MaterialExpressionLocalPosition, -900, 0)
    paper = custom(material, LANTERN, ["LocalPos"], -600, 0, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    MEL.connect_material_expressions(local, "", paper, "LocalPos")
    color = vector_param(material, "Color", (1.0, 0.6, 0.25, 1), -600, 200)
    strength = scalar_param(material, "Strength", 10.0, -600, 400)
    lit = node(material, unreal.MaterialExpressionMultiply, -300, 100)
    MEL.connect_material_expressions(color, "", lit, "A")
    MEL.connect_material_expressions(strength, "", lit, "B")
    product = node(material, unreal.MaterialExpressionMultiply, -100, 50)
    MEL.connect_material_expressions(lit, "", product, "A")
    MEL.connect_material_expressions(paper, "", product, "B")
    MEL.connect_material_property(product, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(material, path)


def build_sky():
    """The day sky as a dome. On desktop the engine can draw the atmosphere without one, but the mobile renderer (the
    web, phones) needs a mesh whose material is marked as the sky, or the sky is black and so is all the sky's light."""
    material, path = fresh("M_GG_Sky")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("is_sky", True)
    air = node(material, unreal.MaterialExpressionSkyAtmosphereViewLuminance, -500, 0)
    disk = node(material, unreal.MaterialExpressionSkyAtmosphereLightDiskLuminance, -500, 200)
    total = node(material, unreal.MaterialExpressionAdd, -250, 100)
    MEL.connect_material_expressions(air, "", total, "A")
    MEL.connect_material_expressions(disk, "", total, "B")
    MEL.connect_material_property(total, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(material, path)


def build_ball():
    material, path = fresh("M_GG_Ball")
    local = node(material, unreal.MaterialExpressionLocalPosition, -900, 0)
    dimples = custom(material, BALL, ["LocalPos"], -600, 0, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    MEL.connect_material_expressions(local, "", dimples, "LocalPos")
    # Dimples darken slightly and roughen, which reads as a golf ball under a key light.
    color = node(material, unreal.MaterialExpressionLinearInterpolate, -300, -100)
    dark = node(material, unreal.MaterialExpressionConstant3Vector, -500, -250,
                constant=unreal.LinearColor(0.62, 0.63, 0.66, 1))
    light = node(material, unreal.MaterialExpressionConstant3Vector, -500, -150,
                 constant=unreal.LinearColor(0.93, 0.93, 0.92, 1))
    MEL.connect_material_expressions(dark, "", color, "A")
    MEL.connect_material_expressions(light, "", color, "B")
    MEL.connect_material_expressions(dimples, "", color, "Alpha")
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = node(material, unreal.MaterialExpressionLinearInterpolate, -300, 150)
    MEL.connect_material_expressions(node(material, unreal.MaterialExpressionConstant, -500, 100, r=0.55), "", rough, "A")
    MEL.connect_material_expressions(node(material, unreal.MaterialExpressionConstant, -500, 200, r=0.18), "", rough, "B")
    MEL.connect_material_expressions(dimples, "", rough, "Alpha")
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    finish(material, path)


TRIPLANAR = r"""
// World-aligned (triplanar) sampling of a scanned surface, so any box or cliff gets seamless, unstretched detail.
float3 P = WorldPos / max(Tile, 1.0);
float3 N = normalize(WorldNormal);
float3 W = pow(abs(N), 4.0);
W /= (W.x + W.y + W.z);

float2 UvX = P.yz;
float2 UvY = P.xz;
float2 UvZ = P.xy;

float3 Base = Texture2DSample(BaseTex, BaseTexSampler, UvX).rgb * W.x
            + Texture2DSample(BaseTex, BaseTexSampler, UvY).rgb * W.y
            + Texture2DSample(BaseTex, BaseTexSampler, UvZ).rgb * W.z;

float3 Orm = Texture2DSample(OrmTex, OrmTexSampler, UvX).rgb * W.x
           + Texture2DSample(OrmTex, OrmTexSampler, UvY).rgb * W.y
           + Texture2DSample(OrmTex, OrmTexSampler, UvZ).rgb * W.z;

// Whiteout blend of the three tangent-space normals into world space.
float3 Tx = UnpackNormalMap(Texture2DSample(NormalTex, NormalTexSampler, UvX)).xyz;
float3 Ty = UnpackNormalMap(Texture2DSample(NormalTex, NormalTexSampler, UvY)).xyz;
float3 Tz = UnpackNormalMap(Texture2DSample(NormalTex, NormalTexSampler, UvZ)).xyz;
Tx = float3(Tx.xy + N.zy, abs(Tx.z) * N.x);
Ty = float3(Ty.xy + N.xz, abs(Ty.z) * N.y);
Tz = float3(Tz.xy + N.xy, abs(Tz.z) * N.z);
NormalOut = normalize(Tx.zyx * W.x + Ty.xzy * W.y + Tz.xyz * W.z);

// Mowing stripes: alternate bands where the grass lies toward or away from the light.
float Stripe = 1.0;
if (StripeWidth > 1.0)
{
    float Band = sin(WorldPos.x * 3.14159265 / StripeWidth);
    Stripe = 1.0 + StripeStrength * smoothstep(-0.15, 0.15, Band) - StripeStrength * 0.5;
}

AoOut = Orm.r;
RoughnessOut = saturate(Orm.g * RoughnessScale * (2.0 - Stripe));
return Base * Tint * Stripe;
"""


def texture_param(material, name, default, sampler, x, y):
    return node(material, unreal.MaterialExpressionTextureObjectParameter, x, y, parameter_name=name,
                texture=unreal.load_asset(default), sampler_type=sampler)


def build_scan():
    """Parent for Megascans surfaces applied to course geometry: world-aligned, tintable."""
    material, path = fresh("M_GG_Scan")
    material.set_editor_property("tangent_space_normal", False)
    scan = "/Game/Fab/Roman_Limestone_Wall/thzldg2qx_tier_1/Textures/T_thzldg2qx_4K_"
    world = node(material, unreal.MaterialExpressionWorldPosition, -1100, -300)
    normal = node(material, unreal.MaterialExpressionVertexNormalWS, -1100, -200)
    base = texture_param(material, "BaseTex", scan + "B", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -1100, -100)
    normal_tex = texture_param(material, "NormalTex", scan + "N", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
                               -1100, 50)
    orm = texture_param(material, "OrmTex", scan + "ORM", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR,
                        -1100, 200)
    tile = scalar_param(material, "Tile", 300.0, -1100, 350)
    tint = vector_param(material, "Tint", (1, 1, 1, 1), -1100, 450)
    rough = scalar_param(material, "RoughnessScale", 1.0, -1100, 600)
    stripe_width = scalar_param(material, "StripeWidth", 0.0, -1100, 700)
    stripe_strength = scalar_param(material, "StripeStrength", 0.0, -1100, 800)
    kinds = unreal.CustomMaterialOutputType
    sample = custom(material, TRIPLANAR,
                    ["WorldPos", "WorldNormal", "BaseTex", "NormalTex", "OrmTex", "Tile", "Tint", "RoughnessScale", "StripeWidth",
                     "StripeStrength"],
                    -600, 0, outputs=[("NormalOut", kinds.CMOT_FLOAT3), ("RoughnessOut", kinds.CMOT_FLOAT1),
                                      ("AoOut", kinds.CMOT_FLOAT1)])
    for source, pin in [(world, "WorldPos"), (normal, "WorldNormal"), (base, "BaseTex"), (normal_tex, "NormalTex"),
                        (orm, "OrmTex"), (tile, "Tile"), (tint, "Tint"), (rough, "RoughnessScale"),
                        (stripe_width, "StripeWidth"), (stripe_strength, "StripeStrength")]:
        MEL.connect_material_expressions(source, "", sample, pin)
    MEL.connect_material_property(sample, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(sample, "NormalOut", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(sample, "RoughnessOut", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(sample, "AoOut", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    finish(material, path)


OCEAN = r"""
// Sum of directional waves (slopes in meters) as a world-space normal. Far from the camera the waves are
// smaller than a pixel, so their slope fades out and roughness rises to stand in for them (no moire).
float2 P = WorldPos.xy / 100.0;
float Distance = length(WorldPos - CameraPos) / 100.0;
float2 Slope = 0;
const float2 Dirs[7] = { float2(1, 0.2), float2(0.7, 0.7), float2(0.3, -0.95), float2(-0.6, 0.8), float2(0.95, -0.3), float2(-0.2, -1), float2(0.5, 0.85) };
const float Lens[7] = { 61.0, 37.0, 19.0, 9.7, 4.9, 2.3, 1.1 };
const float Amps[7] = { 0.55, 0.32, 0.2, 0.1, 0.05, 0.022, 0.01 };
float Lost = 0;
for (int I = 0; I < 7; I++)
{
    float2 D = normalize(Dirs[I]);
    float K = 6.2831853 / Lens[I];
    float Speed = sqrt(9.81 / K);
    float Phase = dot(D, P) * K - Time * Speed * K;
    // A wave stays visible while its length covers a few pixels: roughly Lens * 900 / Distance.
    float Visible = saturate(Lens[I] * 70.0 / max(Distance, 1.0) - 0.5);
    Slope += D * K * Amps[I] * cos(Phase) * Visible;
    Lost += (1.0 - Visible) * K * Amps[I];
}
RoughnessOut = saturate(0.035 + Lost * 0.55);
return normalize(float3(-Slope.x * Choppiness, -Slope.y * Choppiness, 1.0));
"""


def build_ocean():
    material, path = fresh("M_GG_Ocean")
    material.set_editor_property("tangent_space_normal", False)
    # Single Layer Water: the engine's dedicated water path, with depth absorption and its own reflections.
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    water = node(material, unreal.MaterialExpressionSingleLayerWaterMaterialOutput, -100, 500)
    scatter = node(material, unreal.MaterialExpressionConstant3Vector, -500, 500,
                   constant=unreal.LinearColor(0.0006, 0.0016, 0.0018, 1))
    absorb = node(material, unreal.MaterialExpressionConstant3Vector, -500, 600,
                  constant=unreal.LinearColor(0.0045, 0.0012, 0.0008, 1))
    MEL.connect_material_expressions(scatter, "", water, "ScatteringCoefficients")
    MEL.connect_material_expressions(absorb, "", water, "AbsorptionCoefficients")
    MEL.connect_material_expressions(node(material, unreal.MaterialExpressionConstant, -500, 700, r=0.0), "", water, "PhaseG")
    world = node(material, unreal.MaterialExpressionWorldPosition, -900, 0)
    camera = node(material, unreal.MaterialExpressionCameraPositionWS, -900, 80)
    time = node(material, unreal.MaterialExpressionTime, -900, 150)
    chop = scalar_param(material, "Choppiness", 0.8, -900, 300)
    waves = custom(material, OCEAN, ["WorldPos", "CameraPos", "Time", "Choppiness"], -500, 100,
                   outputs=[("RoughnessOut", unreal.CustomMaterialOutputType.CMOT_FLOAT1)])
    for source, pin in [(world, "WorldPos"), (camera, "CameraPos"), (time, "Time"), (chop, "Choppiness")]:
        MEL.connect_material_expressions(source, "", waves, pin)
    MEL.connect_material_property(waves, "", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(waves, "RoughnessOut", unreal.MaterialProperty.MP_ROUGHNESS)
    color = vector_param(material, "WaterColor", (0.003, 0.028, 0.04, 1), -500, -200)
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(scalar_param(material, "Specular", 0.6, -500, 400), "",
                                  unreal.MaterialProperty.MP_SPECULAR)
    finish(material, path)


SALT = r"""
// Salt crust: pale polygons with raised rims, like a dried lake bed. 2D Voronoi on the ground plane.
struct GGCell
{
    float2 Hash(float2 P)
    {
        P = float2(dot(P, float2(127.1, 311.7)), dot(P, float2(269.5, 183.3)));
        return frac(sin(P) * 43758.5453);
    }
};
GGCell H;
float2 P = WorldPos.xy / CellSize;
float2 I = floor(P);
float2 F = frac(P);
float F1 = 8.0;
float F2 = 8.0;
float2 Id = 0;
for (int Y = -1; Y <= 1; Y++)
for (int X = -1; X <= 1; X++)
{
    float2 G = float2(X, Y);
    float2 R = G + H.Hash(I + G) - F;
    float D = dot(R, R);
    if (D < F1) { F2 = F1; F1 = D; Id = I + G; }
    else if (D < F2) { F2 = D; }
}
float Edge = sqrt(F2) - sqrt(F1);
float Rim = 1.0 - smoothstep(0.0, RimWidth, Edge);
float Shade = 1.0 - smoothstep(RimWidth, RimWidth * 3.0, Edge);
float Tone = 0.9 + 0.1 * H.Hash(Id).x;
float Grain = H.Hash(floor(WorldPos.xy * 0.7)).x;
RoughnessOut = lerp(0.62, 0.45, Rim) + (Grain - 0.5) * 0.08;
float3 Color = Tint.rgb * Tone * (0.97 + 0.03 * Grain);
Color = lerp(Color * lerp(1.0, 0.86, Shade), Tint.rgb * 1.08, Rim);
return Color;
"""

MIRROR = r"""
// A film of brine over salt: almost a perfect mirror, with ripples too small to see except in the reflections.
float2 P = WorldPos.xy / 100.0;
float Distance = length(WorldPos - CameraPos) / 100.0;
float2 Slope = 0;
const float2 Dirs[4] = { float2(1, 0.3), float2(-0.4, 1), float2(0.8, -0.6), float2(-0.9, -0.2) };
const float Lens[4] = { 7.0, 3.1, 1.3, 0.6 };
for (int I = 0; I < 4; I++)
{
    float2 D = normalize(Dirs[I]);
    float K = 6.2831853 / Lens[I];
    float Visible = saturate(Lens[I] * 70.0 / max(Distance, 1.0) - 0.5);
    Slope += D * cos(dot(D, P) * K - Time * (0.25 + 0.2 * I)) * Visible;
}
return normalize(float3(-Slope.x * Ripple, -Slope.y * Ripple, 1.0));
"""


def build_salt():
    material, path = fresh("M_GG_Salt")
    world = node(material, unreal.MaterialExpressionWorldPosition, -900, 0)
    cell = scalar_param(material, "CellSize", 130.0, -900, 120)
    rim = scalar_param(material, "RimWidth", 0.045, -900, 220)
    tint = vector_param(material, "Tint", (0.8, 0.78, 0.74, 1), -900, 320)
    crust = custom(material, SALT, ["WorldPos", "CellSize", "RimWidth", "Tint"], -500, 100,
                   outputs=[("RoughnessOut", unreal.CustomMaterialOutputType.CMOT_FLOAT1)])
    for source, pin in [(world, "WorldPos"), (cell, "CellSize"), (rim, "RimWidth"), (tint, "Tint")]:
        MEL.connect_material_expressions(source, "", crust, pin)
    MEL.connect_material_property(crust, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(crust, "RoughnessOut", unreal.MaterialProperty.MP_ROUGHNESS)
    finish(material, path)


def build_mirror():
    material, path = fresh("M_GG_Mirror")
    material.set_editor_property("tangent_space_normal", False)
    world = node(material, unreal.MaterialExpressionWorldPosition, -900, 0)
    camera = node(material, unreal.MaterialExpressionCameraPositionWS, -900, 80)
    time = node(material, unreal.MaterialExpressionTime, -900, 150)
    ripple = scalar_param(material, "Ripple", 0.006, -900, 300)
    waves = custom(material, MIRROR, ["WorldPos", "CameraPos", "Time", "Ripple"], -500, 100)
    for source, pin in [(world, "WorldPos"), (camera, "CameraPos"), (time, "Time"), (ripple, "Ripple")]:
        MEL.connect_material_expressions(source, "", waves, pin)
    MEL.connect_material_property(waves, "", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(vector_param(material, "BaseColor", (0.2, 0.2, 0.19, 1), -500, -200), "",
                                  unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(scalar_param(material, "Roughness", 0.015, -500, 300), "",
                                  unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar_param(material, "Specular", 0.9, -500, 400), "",
                                  unreal.MaterialProperty.MP_SPECULAR)
    finish(material, path)


NIGHT = NOISE + r"""
// A night sky drawn on a dome: deep gradient, a band of milky light, and stars that keep their size.
float3 V = normalize(-CameraDir);
float Up = saturate(V.z);
float3 Sky = lerp(HorizonColor, ZenithColor, pow(Up, 0.45));

// The band: a great circle tilted across the sky, broken up by noise.
float3 Axis = normalize(float3(0.35, -0.5, 0.8));
float Band = exp(-pow(dot(V, Axis) * 4.2, 2.0));
float Dust = N.Fbm(V * 5.0 + 3.0, 5);
Sky += BandColor * Band * smoothstep(0.25, 0.8, Dust) * (0.4 + 0.6 * Up);

float3 Stars = 0;
for (int Layer = 0; Layer < 3; Layer++)
{
    float Scale = 70.0 + Layer * 95.0;
    float3 Cell = floor(V * Scale);
    float3 Local = frac(V * Scale) - 0.5;
    float H = N.Hash(Cell + Layer * 31.7);
    float Threshold = lerp(0.93, 0.86, Band);
    if (H > Threshold)
    {
        float3 Offset = float3(N.Hash(Cell + 1.3), N.Hash(Cell + 2.7), N.Hash(Cell + 4.1)) - 0.5;
        float D2 = dot(Local - Offset * 0.6, Local - Offset * 0.6);
        float Size = lerp(0.0012, 0.01, pow(N.Hash(Cell + 7.9), 7.0));
        float Twinkle = 0.7 + 0.3 * sin(Time * (1.0 + H * 3.0) + H * 50.0);
        float3 Tint = lerp(float3(0.7, 0.82, 1.0), float3(1.0, 0.86, 0.7), N.Hash(Cell + 9.2));
        Stars += Tint * exp(-D2 / Size) * Twinkle;
    }
}
float3 Night = Sky + Stars * StarStrength * smoothstep(-0.02, 0.15, V.z);

// A moon, for the levels that ask for one (MoonSize is its radius as an angle; zero is no moon). It is part of the
// sky rather than a lit ball in the level so that still water and glass reflect it.
if (MoonSize > 0.0)
{
    float3 M = normalize(MoonDir);
    float Angle = acos(clamp(dot(V, M), -1.0, 1.0));
    float3 Right = normalize(cross(float3(0, 0, 1), M));
    float3 Above = cross(M, Right);
    float2 Q = float2(dot(V, Right), dot(V, Above)) / sin(MoonSize);
    float Disc = smoothstep(1.0, 0.985, length(Q)) * step(0.0, dot(V, M));
    // Seas and highlands, then finer mottling; darker toward the limb, as a ball is.
    float Seas = N.Fbm(float3(Q * 1.7 + 4.0, 2.3), 4) / 0.9375;
    float Fine = N.Fbm(float3(Q * 9.0, 7.1), 3) / 0.875;
    float Albedo = lerp(0.42, 1.0, smoothstep(0.38, 0.62, Seas)) * (0.82 + 0.36 * Fine);
    float Limb = pow(saturate(1.0 - dot(Q, Q)), 0.22);
    float Halo = exp(-Angle * Angle / (MoonSize * MoonSize * 5.0)) * 0.22 + exp(-Angle / (MoonSize * 6.0)) * 0.05;
    Night = lerp(Night, MoonColor * Albedo * Limb, Disc) + MoonColor * Halo * (1.0 - Disc) * smoothstep(-0.05, 0.05, V.z);
}

// An aurora, likewise only where asked for. Each ray is walked up through a slab of sky; where it crosses a curtain
// (the middle contour of a slow noise field) it gathers light, one colour at the curtain's foot and another at its top.
// Curtains hang straight down, so seen from below they lean together toward the top of the sky, as real ones do.
if (AuroraStrength > 0.0 && V.z > 0.02)
{
    float3 Glow = 0;
    // Each pixel starts its walk at a different depth, differently every frame, and the picture's own smoothing over
    // time blends the steps away. (Walked in step, fourteen samples drew fourteen thin lines, like a contour map.)
    // Detail is 1 on a computer and 0 in the phone renderer (which the web uses too): there the walk takes a third of
    // the steps over smoother noise, and its dither holds still, since nothing blends frames together.
    int Count = Detail > 0.5 ? 18 : 6;
    float Jitter = N.Hash(V * 791.0 + frac(Time * 7.3) * 13.0 * Detail);
    for (int Step = 0; Step < Count; Step++)
    {
        float T = (Step + Jitter) / Count;
        float2 P = V.xy / V.z * (1.0 + T * 1.3);
        float Field = Detail > 0.5 ? N.Fbm(float3(P.x * 0.45, P.y * 0.15, Time * 0.012), 3) / 0.875
                                   : N.Fbm(float3(P.x * 0.45, P.y * 0.15, Time * 0.012), 2) / 0.75;
        float Sheet = pow(saturate(1.0 - abs(Field * 2.0 - 1.0)), 7.0);
        // Fine upright streaks: quick changes along the curtain, slow ones across it.
        float Rays = 0.35 + 0.65 * N.Noise(float3(P.y * 9.0, P.x * 2.0, Time * 0.05));
        float Profile = exp(-T * 2.6) * smoothstep(0.0, 0.1, T);
        Glow += lerp(AuroraLow, AuroraHigh, T) * Sheet * Rays * Profile / (1.0 + dot(P, P) * 0.015);
    }
    Night += Glow * (AuroraStrength / Count) * smoothstep(0.02, 0.16, V.z);
}
return Night;
"""


def build_night_sky():
    material, path = fresh("M_GG_NightSky")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("is_sky", True)
    camera = node(material, unreal.MaterialExpressionCameraVectorWS, -900, -200)
    horizon = vector_param(material, "HorizonColor", (0.012, 0.02, 0.045, 1), -900, 0)
    zenith = vector_param(material, "ZenithColor", (0.0015, 0.003, 0.012, 1), -900, 150)
    band = vector_param(material, "BandColor", (0.03, 0.035, 0.07, 1), -900, 300)
    stars = scalar_param(material, "StarStrength", 6.0, -900, 420)
    time = node(material, unreal.MaterialExpressionTime, -900, 500)
    # Off unless a level's instance turns them on (the salt at night does: Tools/levels/periscope.py).
    moon_dir = vector_param(material, "MoonDir", (0, 0, 1, 1), -900, 600)
    moon_size = scalar_param(material, "MoonSize", 0.0, -900, 750)
    moon_color = vector_param(material, "MoonColor", (0.8, 0.88, 1.0, 1), -900, 850)
    aurora = scalar_param(material, "AuroraStrength", 0.0, -900, 1000)
    aurora_low = vector_param(material, "AuroraLow", (0.1, 1.0, 0.45, 1), -900, 1100)
    aurora_high = vector_param(material, "AuroraHigh", (0.55, 0.2, 1.0, 1), -900, 1250)
    # 1 on a computer, 0 in the phone renderer: the aurora is walked in fewer steps there.
    detail = node(material, unreal.MaterialExpressionFeatureLevelSwitch, -900, 1400)
    one = node(material, unreal.MaterialExpressionConstant, -1100, 1380, r=1.0)
    zero = node(material, unreal.MaterialExpressionConstant, -1100, 1460, r=0.0)
    for source, pin in [(one, "Default"), (zero, "ES3_1")]:
        if not MEL.connect_material_expressions(source, "", detail, pin):
            unreal.log_error(f"GG: night sky: no '{pin}' pin on the feature level switch")
    sky = custom(material, NIGHT, ["CameraDir", "HorizonColor", "ZenithColor", "BandColor", "StarStrength", "Time",
                                   "MoonDir", "MoonSize", "MoonColor", "AuroraStrength", "AuroraLow", "AuroraHigh",
                                   "Detail"],
                 -500, 100)
    MEL.connect_material_expressions(detail, "", sky, "Detail")
    for source, pin in [(camera, "CameraDir"), (horizon, "HorizonColor"), (zenith, "ZenithColor"), (band, "BandColor"),
                        (stars, "StarStrength"), (time, "Time"), (moon_dir, "MoonDir"), (moon_size, "MoonSize"),
                        (moon_color, "MoonColor"), (aurora, "AuroraStrength"), (aurora_low, "AuroraLow"),
                        (aurora_high, "AuroraHigh")]:
        MEL.connect_material_expressions(source, "", sky, pin)
    MEL.connect_material_property(sky, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(material, path)


GRID = r"""
// The Workshop's table: dark slate ruled like drafting paper. A fine line every Minor centimetres (the squares holes
// are built on) and a bright one every Major. Lines are a pixel or two wide at any distance and fade before they
// would shimmer.
float2 P = WorldPos.xy;
float Dist = length(WorldPos - CameraPos);
float2 A = P / Minor;
float2 WA = max(fwidth(A), 1e-5);
float2 DA = abs(frac(A - 0.5) - 0.5) / WA;
float Fine = (1.0 - saturate(min(DA.x, DA.y))) * saturate(1.0 - max(WA.x, WA.y) * 3.0);
float2 B = P / Major;
float2 WB = max(fwidth(B), 1e-5);
float2 DB = abs(frac(B - 0.5) - 0.5) / WB;
float Bold = (1.0 - saturate(min(DB.x, DB.y) - 0.6)) * saturate(1.0 - max(WB.x, WB.y) * 3.0);
float Far = saturate(1.0 - Dist / 90000.0);
Lines = (Fine * 0.22 + Bold * 0.9) * Far * Far;
// A slow mottling so the slate is not one flat colour.
float Cloud = sin(P.x * 0.0011) * sin(P.y * 0.0013) * 0.5 + 0.5;
// Ink darkens the lines instead of lighting them: ruled paper rather than a lit table.
return Tint.rgb * (0.8 + 0.4 * Cloud) * (1.0 - saturate((Fine * 0.6 + Bold) * Far) * Ink);
"""


def build_grid():
    material, path = fresh("M_GG_Grid")
    world = node(material, unreal.MaterialExpressionWorldPosition, -900, 0)
    camera = node(material, unreal.MaterialExpressionCameraPositionWS, -900, 80)
    minor = scalar_param(material, "Minor", 50.0, -900, 160)
    major = scalar_param(material, "Major", 500.0, -900, 240)
    tint = vector_param(material, "Tint", (0.012, 0.022, 0.045, 1), -900, 320)
    line = vector_param(material, "LineColor", (0.25, 0.6, 1.0, 1), -900, 480)
    strength = scalar_param(material, "Strength", 1.2, -900, 640)
    ink = scalar_param(material, "Ink", 0.0, -900, 720)
    slate = custom(material, GRID, ["WorldPos", "CameraPos", "Minor", "Major", "Tint", "Ink"], -500, 100,
                   outputs=[("Lines", unreal.CustomMaterialOutputType.CMOT_FLOAT1)])
    for source, pin in [(world, "WorldPos"), (camera, "CameraPos"), (minor, "Minor"), (major, "Major"), (tint, "Tint"),
                        (ink, "Ink")]:
        MEL.connect_material_expressions(source, "", slate, pin)
    glow = node(material, unreal.MaterialExpressionMultiply, -200, 300)
    MEL.connect_material_expressions(line, "", glow, "A")
    MEL.connect_material_expressions(slate, "Lines", glow, "B")
    lit = node(material, unreal.MaterialExpressionMultiply, -50, 400)
    MEL.connect_material_expressions(glow, "", lit, "A")
    MEL.connect_material_expressions(strength, "", lit, "B")
    MEL.connect_material_property(slate, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(scalar_param(material, "Roughness", 0.32, -500, 520), "", unreal.MaterialProperty.MP_ROUGHNESS)
    finish(material, path)


BUILDS = {"surface": build_surface, "glow": build_glow, "lantern": build_lantern, "sky": build_sky, "ball": build_ball,
          "ocean": build_ocean, "scan": build_scan, "salt": build_salt, "mirror": build_mirror, "night_sky": build_night_sky,
          "grid": build_grid}
# GG_ONLY (set by the caller) rebuilds just the named materials. A material that is rebuilt resets every instance
# made from it, so adding a new one this way leaves the levels alone.
for build_name in globals().get("GG_ONLY") or BUILDS:
    BUILDS[build_name]()
unreal.log("GG: materials done")
