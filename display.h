#ifndef DISPLAY_H_
# define DISPLAY_H_

#include <interface/vmcs_host/vc_tvservice.h>
#include <gio/gio.h>

#define DISP_DBUS_INTERFACE_NAME "net.raspberry.display"
#define DISP_DBUS_INTERFACE_PATH "/"

#define STATE_VCHI_CONNECTED   (1 << 0)
#define STATE_TV_INITIALISED   (1 << 1)
#define STATE_DBUS_INITIALISED (1 << 2)
#define STATE_DBUS_RUNNING     (1 << 3)
#define STATE_HDMI_RUNNING     (1 << 4)
#define STATE_HDMI_ACTIVE      (1 << 5)

#define DEFAULT_HDMI_FORMAT    HDMI_CEA_720p60
#define DEFAULT_SDTV_MODE      SDTV_MODE_PAL
#define DEFAULT_SDTV_ASPECT    SDTV_ASPECT_16_9

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

#define ARRAY_SIZE(a) (sizeof (a) / sizeof (a[0]))
#define HDMI_FORMATS_COUNT ARRAY_SIZE(hdmi_formats_str)

typedef struct disp_ctx {
    int state;

    struct {
        VCHI_INSTANCE_T     instance;
        VCHI_CONNECTION_T   *connection;
        HDMI_MODE_T         mode;
        HDMI_CEA_RES_CODE_T format;
    } hdmi;

    struct {
        GMainLoop       *loop;
        GDBusNodeInfo   *introspection_data;
        GDBusConnection *connection;
        guint           owner_id;
    } dbus;

} disp_ctx_t;

typedef struct hdmi_infos {
    TV_DEVICE_ID_T device_id;
    TV_SUPPORTED_MODE_NEW_T supported[HDMI_FORMATS_COUNT];
    int count;
} hdmi_infos_t;

typedef struct state {
    int        running;
    int        active;
    int        hdcp;
    const char *format;
    const char *mode;
} state_t;

void display_stop(disp_ctx_t *ctx);

int  display_dbus_init(disp_ctx_t *ctx);
void display_dbus_cleanup(disp_ctx_t *ctx);
void  display_dbus_start(disp_ctx_t *ctx);
void display_dbus_stop(disp_ctx_t *ctx);
void display_dbus_event(disp_ctx_t *ctx, char *event);

int  display_hdmi_start(disp_ctx_t *ctx, HDMI_MODE_T mode);
void display_hdmi_stop(disp_ctx_t *ctx);
int  display_hdmi_get_infos(disp_ctx_t *ctx, hdmi_infos_t *edid);
int  display_hdmi_get_state(disp_ctx_t *ctx, state_t *state);
int  display_hdmi_set_format(disp_ctx_t *ctx, char *format);

#endif /* !DISPLAY.H */
