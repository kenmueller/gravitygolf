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


for build in (build_surface, build_glow, build_ball, build_ocean, build_scan):
    build()
unreal.log("GG: materials done")
