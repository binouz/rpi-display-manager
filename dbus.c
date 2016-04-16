#include <stdio.h>
#include <glib.h>

#include "display.h"

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='net.raspberry.display'>"
  "    <method name='Start'>"
  "      <arg type='s' name='mode' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='Stop'>"
  "    </method>"
  "    <method name='GetInfos'>"
  "      <arg type='a{sv}' name='edid' direction='out'/>"
  "    </method>"
  "    <method name='GetState'>"
  "      <arg type='a{sv}' name='state' direction='out'/>"
  "    </method>"
  "    <method name='SetFormat'>"
  "      <arg type='s' name='format' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <signal name='Event'>"
  "      <arg type='s' name='name'/>"
  "    </signal>"
  "  </interface>"
  "</node>";

static void on_start(disp_ctx_t *ctx, GDBusMethodInvocation *invocation, GVariant *params)
{
  int success = 0;
  GVariant *res = NULL;
  char *mode_str;
  int mode = 0;

  g_variant_get(params, "(&s)", &mode_str);
  mode = display_output_mode_from_str(mode_str);

  if ((mode >= 0) && ((ctx->state & STATE_OUTPUT_RUNNING) || display_output_start(ctx, mode) == 0))
    success = 1;

  res = g_variant_new("(b)", (success == 1) ? TRUE : FALSE);
  g_dbus_method_invocation_return_value(invocation, res);
}

static void on_stop(disp_ctx_t *ctx, GDBusMethodInvocation *invocation)
{
  if (ctx->state & STATE_OUTPUT_RUNNING)
    display_output_stop(ctx);

  g_dbus_method_invocation_return_value(invocation, NULL);
}

static void on_get_infos(disp_ctx_t *ctx, GDBusMethodInvocation *invocation)
{
  GVariant *res = NULL;
  int i;
  GVariantBuilder builder;
  GVariantBuilder fmt_builder;
  disp_infos_t infos;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

  if (display_output_get_infos(ctx, &infos) == 0)
  {
    g_variant_builder_add(&builder, "{sv}", "vendor",
                          g_variant_new("s", infos.vendor));
    g_variant_builder_add(&builder, "{sv}", "name",
                          g_variant_new("s", infos.name));
    g_variant_builder_add(&builder, "{sv}", "serial",
                          g_variant_new("u", infos.serial));

    g_variant_builder_init(&fmt_builder, G_VARIANT_TYPE("as"));
    for (i = 0; i < infos.count; i++)
      g_variant_builder_add(&fmt_builder, "s", infos.formats[i]);
    g_variant_builder_add(&builder, "{sv}", "formats",
                          g_variant_new("as", &fmt_builder));

    display_output_clean_infos(&infos);
  }

  res = g_variant_new("(a{sv})", &builder);
  g_dbus_method_invocation_return_value(invocation, res);
}

static void on_get_state(disp_ctx_t *ctx, GDBusMethodInvocation *invocation)
{
  GVariant *res = NULL;
  GVariantBuilder builder;
  disp_state_t state;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

  if (display_output_get_state(ctx, &state) == 0)
  {
    g_variant_builder_add(&builder, "{sv}", "active",
                          g_variant_new("b", state.active ? TRUE : FALSE));
    g_variant_builder_add(&builder, "{sv}", "running",
                          g_variant_new("b", state.running ? TRUE : FALSE));
    g_variant_builder_add(&builder, "{sv}", "HDCP",
                          g_variant_new("b", state.hdcp ? TRUE : FALSE));
    g_variant_builder_add(&builder, "{sv}", "format",
                          g_variant_new("s", state.active ? state.format : ""));
    g_variant_builder_add(&builder, "{sv}", "mode",
                          g_variant_new("s", state.mode));
  }

  res = g_variant_new("(a{sv})", &builder);
  g_dbus_method_invocation_return_value(invocation, res);
}

static void on_set_format(disp_ctx_t *ctx, GDBusMethodInvocation *invocation, GVariant *params)
{
  int success;
  GVariant *res;
  char *format;

  g_variant_get(params, "(&s)", &format);
  success = display_output_set_format(ctx, format);
  res = g_variant_new("(b)", success == 0 ? TRUE : FALSE);
  g_dbus_method_invocation_return_value(invocation, res);
}

static void on_method_call(GDBusConnection *connection,
                           const gchar *sender,
                           const gchar *path,
                           const gchar *iname,
                           const gchar *mname,
                           GVariant *params,
                           GDBusMethodInvocation *invocation,
                           gpointer user_data)
{
  disp_ctx_t *ctx = (disp_ctx_t*)user_data;
  int success = 0;

  if (g_strcmp0(mname, "Start") == 0)
    on_start(ctx, invocation, params);
  else if (g_strcmp0(mname, "Stop") == 0)
    on_stop(ctx, invocation);
  else if (g_strcmp0(mname, "GetInfos") == 0)
    on_get_infos(ctx, invocation);
  else if (g_strcmp0(mname, "GetState") == 0)
    on_get_state(ctx, invocation);
  else if (g_strcmp0(mname, "SetFormat") == 0)
    on_set_format(ctx, invocation, params);
  else
    g_dbus_method_invocation_return_dbus_error(
      invocation,
      "org.freedesktop.DBus.Error.UnknownMethod",
      "The requested method does not exist");
}

static const GDBusInterfaceVTable interface_vtable =
{
  on_method_call,
  NULL,
  NULL
};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

static void on_name_acquired(GDBusConnection *connection,
                             const gchar *name,
                             gpointer user_data)
{
  int ret;
  disp_ctx_t *ctx = (disp_ctx_t *)user_data;
  GError *error = NULL;

  ctx->dbus.connection = connection;
  ret = g_dbus_connection_register_object(
    connection,
    DISP_DBUS_INTERFACE_PATH,
    ctx->dbus.introspection_data->interfaces[0],
    &interface_vtable,
    user_data,
    NULL,
    &error);

  if (ret == 0)
  {
    fprintf(stderr, "%s : g_dbus_connection_register_object failed (%s)\n", __FUNCTION__, error->message);
    g_error_free (error);
    display_stop(ctx);
  }
}

static void on_name_lost(GDBusConnection *connection,
                         const gchar *name,
                         gpointer user_data)
{
  disp_ctx_t *ctx = (disp_ctx_t *)user_data;

  ctx->dbus.connection = NULL;

  fprintf(stderr, "DBUS name lost, exiting...\n");

  display_stop(ctx);
}

int display_dbus_init(disp_ctx_t *ctx)
{
  GError *error = NULL;

  ctx->dbus.loop = NULL;
  ctx->dbus.connection = NULL;
  ctx->dbus.introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);
  if (ctx->dbus.introspection_data == NULL)
  {
    fprintf(stderr, "%s : g_dbus_node_info_new_for_xml failed (%s)\n", error->message);
    g_error_free (error);
    return 1;
  }

  ctx->dbus.owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                                      DISP_DBUS_INTERFACE_NAME,
                                      G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                      on_bus_acquired,
                                      on_name_acquired,
                                      on_name_lost,
                                      ctx,
                                      NULL);

  return 0;
}

void display_dbus_cleanup(disp_ctx_t *ctx)
{
  g_bus_unown_name(ctx->dbus.owner_id);
  g_dbus_node_info_unref(ctx->dbus.introspection_data);
}

void display_dbus_start(disp_ctx_t *ctx)
{
  if (ctx && !ctx->dbus.loop)
  {
    ctx->state |= STATE_DBUS_RUNNING;
    ctx->dbus.loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(ctx->dbus.loop);
  }
}

void display_dbus_stop(disp_ctx_t *ctx)
{
  if (ctx && ctx->dbus.loop)
  {
    ctx->state &= ~STATE_DBUS_RUNNING;
    g_main_loop_quit(ctx->dbus.loop);
  }
}

void display_dbus_event(disp_ctx_t *ctx, char *name)
{

  if (ctx->dbus.connection)
    g_dbus_connection_emit_signal(
      ctx->dbus.connection,
      NULL,
      DISP_DBUS_INTERFACE_PATH,
      DISP_DBUS_INTERFACE_NAME,
      "Event",
      g_variant_new("(s)", name),
      NULL);
}
