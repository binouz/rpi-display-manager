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
#define STATE_OUTPUT_RUNNING   (1 << 4)
#define STATE_OUTPUT_ACTIVE    (1 << 5)

#define DEFAULT_HDMI_MODE      HDMI_MODE_HDMI
#define DEFAULT_HDMI_FORMAT    HDMI_CEA_720p60

#define DEFAULT_SDTV_MODE      SDTV_MODE_PAL
#define DEFAULT_SDTV_FORMAT    SDTV_ASPECT_16_9

#define FORMAT_MAX_LEN         32

typedef struct disp_ctx {
    int state;

    struct {
        VCHI_INSTANCE_T   instance;
        VCHI_CONNECTION_T *connection;
        int               mode;
        int               format;
    } out;

    struct {
        GMainLoop       *loop;
        GDBusNodeInfo   *introspection_data;
        GDBusConnection *connection;
        guint           owner_id;
    } dbus;

} disp_ctx_t;

typedef struct disp_infos {
    char vendor[EDID_DEVICE_VENDOR_ID_LENGTH+1];
    char name[EDID_DESC_ASCII_STRING_LEN+1];
    int  serial;
    int  count;
    char **formats;
} disp_infos_t;

typedef struct disp_state {
    int        running;
    int        active;
    int        hdcp;
    const char *format;
    const char *mode;
} disp_state_t;

void display_stop(disp_ctx_t *ctx);

int  display_dbus_init(disp_ctx_t *ctx);
void display_dbus_cleanup(disp_ctx_t *ctx);
void display_dbus_start(disp_ctx_t *ctx);
void display_dbus_stop(disp_ctx_t *ctx);
void display_dbus_event(disp_ctx_t *ctx, char *event);

int  display_output_start(disp_ctx_t *ctx, int mode);
void display_output_stop(disp_ctx_t *ctx);
int  display_output_get_infos(disp_ctx_t *ctx, disp_infos_t *edid);
int  display_output_get_state(disp_ctx_t *ctx, disp_state_t *state);
int  display_output_set_format(disp_ctx_t *ctx, char *format);
int  display_output_mode_from_str(char *str);
void display_output_clean_infos(disp_infos_t *infos);

#endif /* !DISPLAY.H */
