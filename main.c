#include <signal.h>
#include <stdio.h>

#include "display.h"

int display_init(disp_ctx_t *ctx)
{
  int ret;

  ctx->state = 0;

  vcos_init();

  ret = vchi_initialise(&ctx->hdmi.instance);
  if (ret != 0)
  {
    fprintf(stderr, "%s : vchi_initialise failed (%d)\n", __FUNCTION__, ret);
    return 1;
  }

  ret = vchi_connect(NULL, 0, ctx->hdmi.instance);
  if (ret != 0)
  {
    fprintf(stderr, "%s : vchi_connect failed (%d)\n", __FUNCTION__, ret);
    return 1;
  }
  ctx->state |= STATE_VCHI_CONNECTED;

  ret = vc_vchi_tv_init(ctx->hdmi.instance, &ctx->hdmi.connection, 1);
  if (ret != 0)
  {
    fprintf(stderr, "%s : vc_vchi_tv_init failed (%d)\n", __FUNCTION__, ret);
    return 1;
  }
  ctx->state |= STATE_TV_INITIALISED;

  ret = display_dbus_init(ctx);
  if (ret != 0)
  {
    fprintf(stderr, "%s : display_dbus_init failed (%d)\n", __FUNCTION__, ret);
    return 1;
  }
  ctx->state |= STATE_DBUS_INITIALISED;

  return 0;
}

void display_cleanup(disp_ctx_t *ctx)
{
  if (ctx->state & STATE_DBUS_INITIALISED)
  {
    display_dbus_cleanup(ctx);
    ctx->state &= ~STATE_DBUS_INITIALISED;
  }

  if (ctx->state & STATE_TV_INITIALISED)
  {
    vc_vchi_tv_stop();
    ctx->state &= ~STATE_TV_INITIALISED;
  }

  if (ctx->state & STATE_VCHI_CONNECTED)
  {
    vchi_disconnect(ctx->hdmi.instance);
    ctx->state &= ~STATE_VCHI_CONNECTED;
  }
}

int display_run(disp_ctx_t *ctx)
{
  display_dbus_start(ctx);

  if (display_hdmi_start(ctx, HDMI_MODE_HDMI) != 0)
  {
    fprintf(stderr, "%s : display_hdmi_start failed\n", __FUNCTION__);
    return 1;
  }

  return 0;
}

void display_stop(disp_ctx_t *ctx)
{
  if (ctx->state & STATE_DBUS_RUNNING)
  {
    display_dbus_stop(ctx);
    ctx->state &= ~STATE_DBUS_RUNNING;
  }

  display_hdmi_stop(ctx);
}

int main(int argc, char **argv)
{
  disp_ctx_t ctx;

  void _sigint(int sn)
  {
    display_stop(&ctx);
  }

  printf("display_init\n");
  if (display_init(&ctx) != 0)
    return 1;

  signal(SIGINT, _sigint);
  signal(SIGTERM, _sigint);

  printf("display_run\n");
  display_run(&ctx);

  display_cleanup(&ctx);

  return 0;
}
