#include <stdio.h>

#include "display.h"

#define TV_SUPPORTED_MODE_T TV_SUPPORTED_MODE_NEW_T
#define vc_tv_hdmi_get_supported_modes vc_tv_hdmi_get_supported_modes_new
#define vc_tv_hdmi_power_on_explicit vc_tv_hdmi_power_on_explicit_new

static char *sdtv_events_str(uint32_t reason)
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

static char *hdmi_events_str(uint32_t reason)
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

static void hdmi_callback(void *callback_data,
                          uint32_t reason,
                          uint32_t param1,
                          uint32_t param2)
{
  disp_ctx_t *ctx = (disp_ctx_t *)callback_data;

  if (ctx->hdmi.mode != HDMI_MODE_OFF)
  {
    if (reason == VC_HDMI_ATTACHED)
      ctx->state |= STATE_HDMI_ACTIVE;
    else if (reason == VC_HDMI_UNPLUGGED)
      ctx->state &= ~STATE_HDMI_ACTIVE;
    display_dbus_event(ctx, sdtv_events_str(reason));
  }
  else
  {
    if (reason == VC_SDTV_ATTACHED)
      ctx->state |= STATE_HDMI_ACTIVE;
    else if (reason == VC_SDTV_UNPLUGGED)
      ctx->state &= ~STATE_HDMI_ACTIVE;
    display_dbus_event(ctx, hdmi_events_str(reason));
  }
}

int display_hdmi_start(disp_ctx_t *ctx, HDMI_MODE_T mode)
{
  vc_tv_register_callback(hdmi_callback, ctx);

  ctx->hdmi.format = DEFAULT_HDMI_FORMAT;

  if (mode == HDMI_MODE_OFF)
  {
    SDTV_OPTIONS_T opts;

    opts.aspect = DEFAULT_SDTV_ASPECT;
    vc_tv_sdtv_power_on(DEFAULT_SDTV_MODE, &opts);
  }
  else
  {
    HDMI_RES_GROUP_T group = HDMI_RES_GROUP_CEA;
    HDMI_CEA_RES_CODE_T code = DEFAULT_HDMI_FORMAT;

    if (vc_tv_hdmi_get_supported_modes_new(group, NULL, 0, &group, &code) == 0)
    {
      group = HDMI_RES_GROUP_CEA;
      code = DEFAULT_HDMI_FORMAT;
    }

    ctx->hdmi.format = code;

    vc_tv_hdmi_power_on_explicit(mode, group, code);
  }

  ctx->hdmi.mode = mode;
  ctx->state |= STATE_HDMI_RUNNING;
}

void display_hdmi_stop(disp_ctx_t *ctx)
{
  vc_tv_unregister_callback(hdmi_callback);
  vc_tv_power_off();
  ctx->state &= ~STATE_HDMI_RUNNING;
}

int display_hdmi_get_infos(disp_ctx_t *ctx, hdmi_infos_t *infos)
{
  if (vc_tv_get_device_id(&infos->device_id) != 0)
  {
    fprintf(stderr, "%s : vc_tv_get_device_id failed\n", __FUNCTION__);
    return 1;
  }

  infos->count = vc_tv_hdmi_get_supported_modes_new(HDMI_RES_GROUP_CEA, infos->supported, HDMI_FORMATS_COUNT, NULL, NULL);
  if (infos->count == 0)
  {
    fprintf(stderr, "%s : vc_tv_hdmi_get_supported_modes_new failed\n", __FUNCTION__);
    return 1;
  }

  return 0;
}

int display_hdmi_get_state(disp_ctx_t *ctx, state_t *state)
{
  state->running = ctx->state & STATE_HDMI_RUNNING;
  state->active = ctx->state & STATE_HDMI_ACTIVE;
  state->hdcp = 0;
  state->format = hdmi_formats_str[ctx->hdmi.format];
  switch (ctx->hdmi.mode)
  {
    case HDMI_MODE_HDMI:
      state->mode = "HDMI";
      break;
    case HDMI_MODE_DVI:
      state->mode = "DVI";
      break;
    case HDMI_MODE_3D:
      state->mode = "3D";
      break;
    default:
      state->mode = "SDTV";
      break;
  }
}

/* TODO : Handle SDTV */
int display_hdmi_set_format(disp_ctx_t *ctx, char *format)
{
  int i;

  for (i = 0; i < HDMI_FORMATS_COUNT; i++)
  {
    if (strcmp(format, hdmi_formats_str[i]))
      continue;

    if (vc_tv_hdmi_mode_supported(HDMI_RES_GROUP_CEA, i) > 0)
    {
      vc_tv_hdmi_power_on_explicit(ctx->hdmi.mode, HDMI_RES_GROUP_CEA, i);
      ctx->hdmi.format = i;
      return 0;
    }
    else
    {
      fprintf(stderr, "%s : Unsupported format '%s'\n", __FUNCTION__, format);
      return 1;
    }
  }

  fprintf(stderr, "%s : Unknown format '%s'\n", __FUNCTION__, format);
  return 1;
}
