
#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("disque-obs", "fr-FR")

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Disque OBS - Rotation d'image";
}

static const char *disque_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "Disque";
}

static void *disque_create(obs_data_t *settings,
                           obs_source_t *source)
{
    UNUSED_PARAMETER(settings);
    return source;
}

static void disque_destroy(void *data)
{
    UNUSED_PARAMETER(data);
}

static struct obs_source_info disque_filter = {
    .id = "disque_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = disque_get_name,
    .create = disque_create,
    .destroy = disque_destroy,
};

bool obs_module_load(void)
{
    obs_register_source(&disque_filter);
    return true;
}
