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

PLANET = NOISE + r"""
float3 D = normalize(LocalPos);
float3 Base;
float Rough = 0.85;
int Kind = (int)round(Style);

if (Kind == 2)
{
    // Gas giant: latitude bands warped by turbulence, slowly drifting, with one storm.
    float Warp = N.Fbm(D * 3.0 + float3(0, 0, Time * 0.015), 5);
    float Lat = D.z + (Warp - 0.5) * 0.18;
    float Bands = sin(Lat * 22.0) * 0.5 + 0.5;
    float Fine = sin(Lat * 71.0 + Warp * 6.0) * 0.5 + 0.5;
    Base = lerp(ColorA, ColorB, smoothstep(0.2, 0.8, Bands));
    Base = lerp(Base, ColorC, smoothstep(0.55, 1.0, Fine) * 0.45);
    float3 StormDir = normalize(float3(0.55, -0.6, -0.35));
    float Storm = smoothstep(0.17, 0.05, distance(D, StormDir) + (Warp - 0.5) * 0.05);
    Base = lerp(Base, ColorC * 1.15, Storm * 0.85);
    Rough = 0.55;
}
else if (Kind == 3)
{
    // Ice moon: pale shell, darker frozen seas, glowing cracks.
    float Seas = N.Fbm(D * 2.5, 4);
    float Crack = 1.0 - abs(N.Noise(D * 9.0) * 2.0 - 1.0);
    Crack = pow(saturate(Crack), 18.0);
    Base = lerp(ColorA, ColorB, smoothstep(0.45, 0.6, Seas));
    Base = lerp(Base, ColorC, Crack * 0.8);
    Rough = lerp(0.12, 0.35, Seas);
}
else
{
    // Grassy worlds. The green (Kind 1) gets mowing stripes, like a real putting green.
    float Patches = N.Fbm(D * 4.0, 5);
    float Detail = N.Fbm(D * 28.0, 3);
    Base = lerp(ColorA, ColorB, smoothstep(0.35, 0.7, Patches));
    Base *= lerp(0.85, 1.12, Detail);
    if (Kind == 1)
    {
        float Lon = atan2(D.y, D.x);
        float Stripe = step(0.0, sin(Lon * 14.0));
        Base *= lerp(0.88, 1.12, Stripe);
    }
    else
    {
        // A few rocky outcrops on the meadow.
        float Rock = smoothstep(0.66, 0.72, N.Fbm(D * 6.0 + 3.1, 4));
        Base = lerp(Base, ColorC, Rock);
    }
    Rough = 0.9;
}

RoughnessOut = Rough;
return Base;
"""

SKY = NOISE + r"""
float3 V = normalize(-CameraDir);

// Nebula: two layers of soft colored clouds.
float Cloud = N.Fbm(V * 2.2 + 4.0, 6);
float Wisps = N.Fbm(V * 5.5 + Cloud * 1.5, 5);
float3 Nebula = NebulaA * smoothstep(0.35, 0.85, Cloud) + NebulaB * smoothstep(0.45, 0.95, Wisps) * 0.7;
Nebula *= NebulaStrength;

// Stars: one candidate per cell on a few grid scales, sized and tinted by hash.
float3 Stars = 0;
for (int Layer = 0; Layer < 3; Layer++)
{
    float Scale = 90.0 + Layer * 110.0;
    float3 Cell = floor(V * Scale);
    float3 Local = frac(V * Scale) - 0.5;
    float H = N.Hash(Cell + Layer * 31.7);
    if (H > 0.94)
    {
        float3 Offset = float3(N.Hash(Cell + 1.3), N.Hash(Cell + 2.7), N.Hash(Cell + 4.1)) - 0.5;
        float D2 = dot(Local - Offset * 0.6, Local - Offset * 0.6);
        float Size = lerp(0.0015, 0.012, pow(N.Hash(Cell + 7.9), 6.0));
        float Twinkle = 0.75 + 0.25 * sin(Time * (1.0 + H * 3.0) + H * 50.0);
        float3 Tint = lerp(float3(0.75, 0.85, 1.0), float3(1.0, 0.85, 0.7), N.Hash(Cell + 9.2));
        Stars += Tint * exp(-D2 / Size) * Twinkle * StarStrength;
    }
}

return Nebula + Stars;
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
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = vector_param(material, "Color", (1, 1, 1, 1), -600, 0)
    strength = scalar_param(material, "Strength", 10.0, -600, 200)
    product = node(material, unreal.MaterialExpressionMultiply, -300, 50)
    MEL.connect_material_expressions(color, "", product, "A")
    MEL.connect_material_expressions(strength, "", product, "B")
    MEL.connect_material_property(product, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(material, path)


def build_planet():
    material, path = fresh("M_GG_Planet")
    local = node(material, unreal.MaterialExpressionLocalPosition, -1200, -300)
    style = scalar_param(material, "Style", 0.0, -1200, -150)
    a = vector_param(material, "ColorA", (0.16, 0.42, 0.14, 1), -1200, 0)
    b = vector_param(material, "ColorB", (0.24, 0.55, 0.2, 1), -1200, 150)
    c = vector_param(material, "ColorC", (0.4, 0.38, 0.35, 1), -1200, 300)
    time = node(material, unreal.MaterialExpressionTime, -1200, 450)
    surface = custom(material, PLANET, ["LocalPos", "Style", "ColorA", "ColorB", "ColorC", "Time"], -700, 0,
                     outputs=[("RoughnessOut", unreal.CustomMaterialOutputType.CMOT_FLOAT1)])
    for source, pin in [(local, "LocalPos"), (style, "Style"), (a, "ColorA"), (b, "ColorB"), (c, "ColorC"),
                        (time, "Time")]:
        MEL.connect_material_expressions(source, "", surface, pin)
    MEL.connect_material_property(surface, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(surface, "RoughnessOut", unreal.MaterialProperty.MP_ROUGHNESS)

    # Atmosphere: a soft rim glow that reads as air against black space.
    fresnel = node(material, unreal.MaterialExpressionFresnel, -700, 400, exponent=3.5, base_reflect_fraction=0.0)
    air = vector_param(material, "AtmosphereColor", (0.35, 0.6, 1.0, 1), -700, 550)
    air_strength = scalar_param(material, "AtmosphereStrength", 3.0, -700, 700)
    rim = node(material, unreal.MaterialExpressionMultiply, -400, 450)
    glow = node(material, unreal.MaterialExpressionMultiply, -200, 500)
    MEL.connect_material_expressions(fresnel, "", rim, "A")
    MEL.connect_material_expressions(air, "", rim, "B")
    MEL.connect_material_expressions(rim, "", glow, "A")
    MEL.connect_material_expressions(air_strength, "", glow, "B")
    MEL.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(material, path)


def build_sky():
    material, path = fresh("M_GG_Sky")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("is_sky", True)
    camera = node(material, unreal.MaterialExpressionCameraVectorWS, -900, -200)
    nebula_a = vector_param(material, "NebulaA", (0.08, 0.03, 0.16, 1), -900, 0)
    nebula_b = vector_param(material, "NebulaB", (0.02, 0.09, 0.14, 1), -900, 150)
    nebula_strength = scalar_param(material, "NebulaStrength", 1.0, -900, 300)
    star_strength = scalar_param(material, "StarStrength", 30.0, -900, 400)
    time = node(material, unreal.MaterialExpressionTime, -900, 500)
    sky = custom(material, SKY, ["CameraDir", "NebulaA", "NebulaB", "NebulaStrength", "StarStrength", "Time"],
                 -500, 100)
    for source, pin in [(camera, "CameraDir"), (nebula_a, "NebulaA"), (nebula_b, "NebulaB"),
                        (nebula_strength, "NebulaStrength"), (star_strength, "StarStrength"), (time, "Time")]:
        MEL.connect_material_expressions(source, "", sky, pin)
    MEL.connect_material_property(sky, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
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


OCEAN = r"""
// Sum of directional waves (world-space slopes in meters) plus fine noise ripples, as a world-space normal.
float2 P = WorldPos.xy / 100.0;
float2 Slope = 0;
const float2 Dirs[6] = { float2(1, 0.2), float2(0.7, 0.7), float2(0.3, -0.95), float2(-0.6, 0.8), float2(0.95, -0.3), float2(-0.2, -1) };
const float Lens[6] = { 23.0, 11.0, 7.3, 4.1, 2.3, 1.1 };
const float Amps[6] = { 0.22, 0.12, 0.07, 0.04, 0.02, 0.01 };
for (int I = 0; I < 6; I++)
{
    float2 D = normalize(Dirs[I]);
    float K = 6.2831853 / Lens[I];
    float Speed = sqrt(9.81 / K);
    float Phase = dot(D, P) * K - Time * Speed * K;
    Slope += D * K * Amps[I] * cos(Phase);
}
return normalize(float3(-Slope.x * Choppiness, -Slope.y * Choppiness, 1.0));
"""


def build_ocean():
    material, path = fresh("M_GG_Ocean")
    material.set_editor_property("tangent_space_normal", False)
    world = node(material, unreal.MaterialExpressionWorldPosition, -900, 0)
    time = node(material, unreal.MaterialExpressionTime, -900, 150)
    chop = scalar_param(material, "Choppiness", 0.6, -900, 300)
    waves = custom(material, OCEAN, ["WorldPos", "Time", "Choppiness"], -500, 100)
    for source, pin in [(world, "WorldPos"), (time, "Time"), (chop, "Choppiness")]:
        MEL.connect_material_expressions(source, "", waves, pin)
    MEL.connect_material_property(waves, "", unreal.MaterialProperty.MP_NORMAL)
    color = vector_param(material, "WaterColor", (0.004, 0.03, 0.045, 1), -500, -200)
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(scalar_param(material, "Roughness", 0.035, -500, 300), "",
                                  unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar_param(material, "Specular", 0.6, -500, 400), "",
                                  unreal.MaterialProperty.MP_SPECULAR)
    finish(material, path)


for build in (build_surface, build_glow, build_planet, build_sky, build_ball, build_ocean):
    build()
unreal.log("GG: materials done")
