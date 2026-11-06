
#include <obs-module.h>
#include <graphics/graphics.h>
#include <util/platform.h>
#include <math.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("disque-obs", "en-US")

struct disque_filter {
    obs_source_t *source;
    gs_effect_t *effect;
    float angle;
    double speed;
    int direction;
    bool enabled;
};

static const char *shader_code =
    "uniform float4x4 ViewProj;\n"
    "uniform texture2d image;\n"
    "uniform float rotation;\n"
    "uniform float aspect;\n"
    "sampler_state textureSampler {\n"
    "    Filter = Linear;\n"
    "    AddressU = Clamp;\n"
    "    AddressV = Clamp;\n"
    "};\n"
    "struct VertData {\n"
    "    float4 pos : POSITION;\n"
    "    float2 uv : TEXCOORD0;\n"
    "};\n"
    "VertData VSDefault(VertData v) {\n"
    "    VertData o;\n"
    "    o.pos = mul(float4(v.pos.xyz, 1.0), ViewProj);\n"
    "    o.uv = v.uv;\n"
    "    return o;\n"
    "}\n"
    "float4 PSRotate(VertData v) : TARGET {\n"
    "    float c = cos(rotation);\n"
    "    float s = sin(rotation);\n"
    "    float2 p = v.uv - float2(0.5, 0.5);\n"
    "    p.x *= aspect;\n"
    "    float2 q;\n"
    "    q.x = c * p.x + s * p.y;\n"
    "    q.y = -s * p.x + c * p.y;\n"
    "    q.x /= aspect;\n"
    "    q += float2(0.5, 0.5);\n"
    "    if (q.x < 0.0 || q.x > 1.0 ||\n"
    "        q.y < 0.0 || q.y > 1.0)\n"
    "        return float4(0.0, 0.0, 0.0, 0.0);\n"
    "    return image.Sample(textureSampler, q);\n"
    "}\n"
    "technique Draw {\n"
    "    pass {\n"
    "        vertex_shader = VSDefault(v);\n"
    "        pixel_shader = PSRotate(v);\n"
    "    }\n"
    "}\n";

static const char *disque_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "Disque - Rotation";
}

static void disque_defaults(obs_data_t *settings)
{
    obs_data_set_default_double(settings, "speed", 30.0);
    obs_data_set_default_int(settings, "direction", 1);
    obs_data_set_default_bool(settings, "enabled", true);
}

static obs_properties_t *disque_properties(void *unused)
{
    UNUSED_PARAMETER(unused);

    obs_properties_t *props = obs_properties_create();

    obs_properties_add_bool(
        props, "enabled", "Activer la rotation");

    obs_properties_add_float_slider(
        props, "speed", "Vitesse (degres/seconde)",
        0.0, 360.0, 1.0);

    obs_property_t *direction = obs_properties_add_list(
        props, "direction", "Sens de rotation",
        OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

    obs_property_list_add_int(direction, "Droite", 1);
    obs_property_list_add_int(direction, "Gauche", -1);

    return props;
}

static void disque_update(void *data, obs_data_t *settings)
{
    struct disque_filter *filter = data;

    filter->speed =
        obs_data_get_double(settings, "speed");

    filter->direction =
        (int)obs_data_get_int(settings, "direction");

    filter->enabled =
        obs_data_get_bool(settings, "enabled");
}

static void *disque_create(
    obs_data_t *settings, obs_source_t *source)
{
    struct disque_filter *filter =
        bzalloc(sizeof(struct disque_filter));

    filter->source = source;
    filter->angle = 0.0f;

    obs_enter_graphics();

    char *errors = NULL;
    filter->effect = gs_effect_create(
        shader_code, "disque.effect", &errors);

    if (errors) {
        blog(LOG_WARNING,
             "[Disque OBS] Shader: %s", errors);
        bfree(errors);
    }

    obs_leave_graphics();

    if (!filter->effect) {
        bfree(filter);
        return NULL;
    }

    disque_update(filter, settings);
    return filter;
}

static void disque_destroy(void *data)
{
    struct disque_filter *filter = data;

    if (!filter)
        return;

    obs_enter_graphics();

    if (filter->effect)
        gs_effect_destroy(filter->effect);

    obs_leave_graphics();

    bfree(filter);
}

static void disque_tick(void *data, float seconds)
{
    struct disque_filter *filter = data;

    if (!filter->enabled)
        return;

    filter->angle +=
        (float)(filter->speed *
                filter->direction * seconds);

    filter->angle = fmodf(filter->angle, 360.0f);
}

static void disque_render(void *data, gs_effect_t *unused)
{
    UNUSED_PARAMETER(unused);

    struct disque_filter *filter = data;

    obs_source_t *target =
        obs_filter_get_target(filter->source);

    if (!target)
        return;

    uint32_t width = obs_source_get_width(target);
    uint32_t height = obs_source_get_height(target);

    if (!width || !height)
        return;

    if (!filter->enabled) {
        obs_source_skip_video_filter(filter->source);
        return;
    }

    if (!obs_source_process_filter_begin(
            filter->source, GS_RGBA,
            OBS_NO_DIRECT_RENDERING))
        return;

    gs_eparam_t *rotation_param =
        gs_effect_get_param_by_name(
            filter->effect, "rotation");

    gs_eparam_t *aspect_param =
        gs_effect_get_param_by_name(
            filter->effect, "aspect");

    if (rotation_param)
        gs_effect_set_float(
            rotation_param,
            filter->angle * 0.017453292519943295f);

    if (aspect_param)
        gs_effect_set_float(
            aspect_param,
            (float)width / (float)height);

    obs_source_process_filter_end(
        filter->source, filter->effect,
        width, height);
}

static struct obs_source_info disque_info = {
    .id = "disque_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = disque_name,
    .create = disque_create,
    .destroy = disque_destroy,
    .get_defaults = disque_defaults,
    .get_properties = disque_properties,
    .update = disque_update,
    .video_tick = disque_tick,
    .video_render = disque_render,
};

bool obs_module_load(void)
{
    obs_register_source(&disque_info);
    blog(LOG_INFO, "[Disque OBS] Plugin charge");
    return true;
}

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Rotation d'image autour de son centre";
}
