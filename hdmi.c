#include <stdio.h>

#include "display.h"

#define TV_SUPPORTED_MODE_T TV_SUPPORTED_MODE_NEW_T
#define vc_tv_hdmi_get_supported_modes vc_tv_hdmi_get_supported_modes_new
#define vc_tv_hdmi_power_on_explicit vc_tv_hdmi_power_on_explicit_new

static const char *hdmi_modes_str[] = {
   "OFF",
   "DVI",
   "HDMI",
   "3D"
};

static const char *hdmi_formats_str[] = {
  "",
  "VGA",
  "480p60",
  "480p60H",
  "720p60",
  "1080i60",
  "480i60",
  "480i60H",
  "240p60",
  "240p60H",
  "480i60",
  "480i60",
  "240p60",
  "240p60",
  "480p60",
  "480p60",
  "1080p60",
  "576p50",
  "576p50H",
  "720p50",
  "1080i50",
  "576i50",
  "576i50H",
  "288p50",
  "288p50H",
  "576i50",
  "576i50",
  "288p50",
  "288p50",
  "576p50",
  "576p50",
  "1080p50",
  "1080p24",
  "1080p25",
  "1080p30",
  "480p60",
  "480p60",
  "576p50",
  "576p50",
  "1080i50",
  "1080i100",
  "720p100",
  "576p100",
  "576p100H",
  "576i100",
  "576i100H",
  "1080i120",
  "720p120",
  "480p120",
  "480p120H",
  "480i120",
  "480i120H",
  "576p200",
  "576p200H",
  "576i200",
  "576i200H",
  "480p240",
  "480p240H",
  "480i240",
  "480i240H",
  "720p24",
  "720p25",
  "720p30",
  "1080p120",
  "1080p100"
};

static const char *sdtv_modes_str[] = {
  "NTSC",
  "NTSC_J",
  "PAL",
  "PAL_M"
};

static const char *sdtv_formats_str[] = {
  "UNKNOWN",
  "4_3",
  "14_9",
  "16_9"
};

#define ARRAY_SIZE(a) (sizeof (a) / sizeof (a[0]))
#define HDMI_MODES_COUNT   ARRAY_SIZE(hdmi_modes_str)
#define SDTV_MODES_COUNT   ARRAY_SIZE(sdtv_modes_str)
#define HDMI_FORMATS_COUNT ARRAY_SIZE(hdmi_formats_str)
#define SDTV_FORMATS_COUNT ARRAY_SIZE(sdtv_formats_str)

static inline int is_hdmi_mode(int mode)
{
  return (mode == HDMI_MODE_OFF  ||
          mode == HDMI_MODE_HDMI ||
          mode == HDMI_MODE_DVI  ||
          mode == HDMI_MODE_3D);
}

static char *hdmi_events_str(uint32_t reason)
{
  switch (reason)
  {
    case VC_HDMI_UNPLUGGED:
      return "VC_HDMI_UNPLUGGED";
    case VC_HDMI_ATTACHED:
      return "VC_HDMI_ATTACHED";
    case VC_HDMI_DVI:
      return "VC_HDMI_DVI";
    case VC_HDMI_HDMI:
      return "VC_HDMI_HDMI";
    case VC_HDMI_HDCP_UNAUTH:
      return "VC_HDMI_HDCP_UNAUTH";
    case VC_HDMI_HDCP_AUTH:
      return "VC_HDMI_HDCP_AUTH";
    case VC_HDMI_HDCP_KEY_DOWNLOAD:
      return "VC_HDMI_HDCP_KEY_DOWNLOAD";
    case VC_HDMI_HDCP_SRM_DOWNLOAD:
      return "VC_HDMI_HDCP_SRM_DOWNLOAD";
    case VC_HDMI_CHANGING_MODE:
      return "VC_HDMI_CHANGING_MODE";
    default :
      return "";
  }
}

static char *sdtv_events_str(uint32_t reason)
{
  switch (reason)
  {
    case VC_SDTV_UNPLUGGED:
      return "VC_SDTV_UNPLUGGED";
    case VC_SDTV_ATTACHED:
      return "VC_SDTV_ATTACHED";
    case VC_SDTV_NTSC:
      return "VC_SDTV_NTSC";
    case VC_SDTV_PAL:
      return "VC_SDTV_PAL";
    case VC_SDTV_CP_INACTIVE:
      return "VC_SDTV_CP_INACTIVE";
    case VC_SDTV_CP_ACTIVE:
      return "VC_SDTV_CP_ACTIVE";
    default :
      return "";
  }
}

static void output_callback(void *callback_data,
                            uint32_t reason,
                            uint32_t param1,
                            uint32_t param2)
{
  disp_ctx_t *ctx = (disp_ctx_t *)callback_data;

  if (is_hdmi_mode(ctx->out.mode))
  {
    if (reason == VC_HDMI_DVI || reason == VC_HDMI_HDMI)
      ctx->state |= STATE_OUTPUT_ACTIVE;
    else if (reason == VC_HDMI_UNPLUGGED)
      ctx->state &= ~STATE_OUTPUT_ACTIVE;
    display_dbus_event(ctx, hdmi_events_str(reason));
  }
  else
  {
    if (reason == VC_SDTV_NTSC || reason == VC_SDTV_PAL)
      ctx->state |= STATE_OUTPUT_ACTIVE;
    else if (reason == VC_SDTV_UNPLUGGED)
      ctx->state &= ~STATE_OUTPUT_ACTIVE;
    display_dbus_event(ctx, sdtv_events_str(reason));
  }
}

int display_output_start(disp_ctx_t *ctx, int mode)
{
  TV_GET_STATE_RESP_T state;

  vc_tv_power_off();

  vc_tv_register_callback(output_callback, ctx);

  if (is_hdmi_mode(mode))
  {
    vc_tv_hdmi_power_on_explicit(mode, HDMI_RES_GROUP_CEA, DEFAULT_HDMI_FORMAT);
    ctx->out.format = DEFAULT_HDMI_FORMAT;
  }
  else
  {
    SDTV_OPTIONS_T opts;

    ctx->out.format = opts.aspect = DEFAULT_SDTV_FORMAT;
    vc_tv_sdtv_power_on(mode, &opts);
  }

  ctx->out.mode = mode;
  ctx->state |= STATE_OUTPUT_RUNNING;

  if (vc_tv_get_state(&state) != 0)
  {
    fprintf(stderr, "%s : vc_tv_get_state failed\n", __FUNCTION__);
    return 1;
  }

  if ((state.state & VC_SDTV_ATTACHED) || (state.state & VC_HDMI_ATTACHED))
    ctx->state |= STATE_OUTPUT_ACTIVE;
  else
    ctx->state &= ~STATE_OUTPUT_ACTIVE;

  return 0;
}

void display_output_stop(disp_ctx_t *ctx)
{
  vc_tv_unregister_callback(output_callback);
  vc_tv_power_off();
  ctx->state &= ~STATE_OUTPUT_ACTIVE;
  ctx->state &= ~STATE_OUTPUT_RUNNING;
}

int display_output_get_infos(disp_ctx_t *ctx, disp_infos_t *infos)
{
  int i;
  TV_DEVICE_ID_T device_id;
  TV_SUPPORTED_MODE_NEW_T supported[HDMI_FORMATS_COUNT];
  char *format;

  if (!is_hdmi_mode(ctx->out.mode))
    return 1;

  if (vc_tv_get_device_id(&device_id) != 0)
  {
    fprintf(stderr, "%s : vc_tv_get_device_id failed\n", __FUNCTION__);
    return 1;
  }

  memcpy(infos->vendor, device_id.vendor, EDID_DEVICE_VENDOR_ID_LENGTH+1);
  memcpy(infos->name, device_id.monitor_name, EDID_DESC_ASCII_STRING_LEN+1);
  infos->serial = device_id.serial_num;

  infos->count = vc_tv_hdmi_get_supported_modes_new(HDMI_RES_GROUP_CEA, supported, HDMI_FORMATS_COUNT, NULL, NULL);
  if (infos->count == 0)
  {
    fprintf(stderr, "%s : vc_tv_hdmi_get_supported_modes_new failed\n", __FUNCTION__);
    return 1;
  }

  infos->formats = malloc(infos->count * sizeof (char *));
  for (i = 0; i < infos->count; i++)
  {
    format = malloc(FORMAT_MAX_LEN + 1);
    snprintf(format, FORMAT_MAX_LEN, "%d%c%d", supported[i].height, supported[i].scan_mode ? 'i' : 'p', supported[i].frame_rate);
    format[FORMAT_MAX_LEN] = 0;
    infos->formats[i] = format;
  }

  return 0;
}

int display_output_get_state(disp_ctx_t *ctx, disp_state_t *state)
{
  state->running = ctx->state & STATE_OUTPUT_RUNNING;
  state->active = ctx->state & STATE_OUTPUT_ACTIVE;
  state->hdcp = 0;

  if (is_hdmi_mode(ctx->out.mode))
  {
    state->format = hdmi_formats_str[ctx->out.format];
    state->mode = hdmi_modes_str[ctx->out.mode];
  }
  else
  {
    state->format = sdtv_formats_str[ctx->out.format];
    state->mode = sdtv_modes_str[ctx->out.mode];
  }

  return 0;
}

int display_output_set_format(disp_ctx_t *ctx, char *format)
{
  int i;

  if (is_hdmi_mode(ctx->out.mode))
  {
    for (i = 0; i < HDMI_FORMATS_COUNT; i++)
    {
      if (strcmp(format, hdmi_formats_str[i]))
        continue;

      if (vc_tv_hdmi_mode_supported(HDMI_RES_GROUP_CEA, i) > 0)
      {
        vc_tv_hdmi_power_on_explicit(ctx->out.mode, HDMI_RES_GROUP_CEA, i);
        ctx->out.format = i;
        return 0;
      }
      else
      {
        fprintf(stderr, "%s : Unsupported format '%s'\n", __FUNCTION__, format);
        return 1;
      }
    }
  }
  else
  {
    for (i = 0; i < SDTV_FORMATS_COUNT; i++)
    {
      if (strcmp(format, sdtv_formats_str[i]))
        continue;

      SDTV_OPTIONS_T opts;

      ctx->out.format = opts.aspect = i;
      vc_tv_sdtv_power_on(ctx->out.mode, &opts);
    }
  }

  fprintf(stderr, "%s : Unknown format '%s'\n", __FUNCTION__, format);
  return 1;
}

int display_output_mode_from_str(char *str)
{
  int i;

  for (i = 0; i < HDMI_MODES_COUNT; i++)
  {
    if (strcmp(str, hdmi_modes_str[i]) == 0)
      return i;
  }

  for (i = 0; i < SDTV_MODES_COUNT; i++)
  {
    if (strcmp(str, sdtv_modes_str[i]) == 0)
      return i;
  }

  fprintf(stderr, "%s : Unknown mode '%s'\n", __FUNCTION__, str);
  return -1;
}

void display_output_clean_infos(disp_infos_t *infos)
{
  int i = 0;

  for (i = 0; i < infos->count; i++)
    free(infos->formats[i]);

  free(infos->formats);
}
