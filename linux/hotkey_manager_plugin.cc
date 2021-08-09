#include "include/hotkey_manager/hotkey_manager_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>

#include <cstring>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define HOTKEY_MANAGER_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), hotkey_manager_plugin_get_type(), \
                              HotkeyManagerPlugin))

struct _HotkeyManagerPlugin
{
  GObject parent_instance;
  FlPluginRegistrar *registrar;
};

// Gets the window being controlled.
GtkWindow *get_window(HotkeyManagerPlugin *self)
{
  FlView *view = fl_plugin_registrar_get_view(self->registrar);
  if (view == nullptr)
    return nullptr;

  return GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(view)));
}

G_DEFINE_TYPE(HotkeyManagerPlugin, hotkey_manager_plugin, g_object_get_type())

static FlMethodResponse *hkm_register(HotkeyManagerPlugin *self,
                                      FlValue *args)
{
  Display *display = XOpenDisplay(0);
  Window rootWindow = DefaultRootWindow(display);
  int keyCode = XKeysymToKeycode(display, XK_Y);
  unsigned int modifiers = ControlMask;

  XGrabKey(display, keyCode, modifiers, rootWindow, False, GrabModeAsync, GrabModeAsync);
  XSelectInput(display, rootWindow, KeyPressMask);

  XEvent event;
  printf("Just quit the app with CTRL+C from the terminal. Use CTRL+SHIFT+Y to start/stop\n");
  while (true)
  {
    XNextEvent(display, &event);
    switch (event.type)
    {
    case KeyPress:
      printf("Hotkey (CTRL+SHIFT+Y) -> ");
      break;
    }
  }

  // We only quit by CTRL+C
  // I think there is no real reason to complicate this with an other combo for quitting!
  printf("THIS SHOULD NEVER GET PRINTED!");

  return FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(true)));
}

static FlMethodResponse *hkm_unregister(HotkeyManagerPlugin *self,
                                        FlValue *args)
{

  return FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(true)));
}

// Called when a method call is received from Flutter.
static void hotkey_manager_plugin_handle_method_call(
    HotkeyManagerPlugin *self,
    FlMethodCall *method_call)
{
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar *method = fl_method_call_get_name(method_call);
  FlValue *args = fl_method_call_get_args(method_call);

  if (strcmp(method, "getPlatformVersion") == 0)
  {
    struct utsname uname_data = {};
    uname(&uname_data);
    g_autofree gchar *version = g_strdup_printf("Linux %s", uname_data.version);
    g_autoptr(FlValue) result = fl_value_new_string(version);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  }
  else if (strcmp(method, "register") == 0)
  {
    response = hkm_register(self, args);
  }
  else if (strcmp(method, "unregister") == 0)
  {
    response = hkm_unregister(self, args);
  }
  else
  {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void hotkey_manager_plugin_dispose(GObject *object)
{
  G_OBJECT_CLASS(hotkey_manager_plugin_parent_class)->dispose(object);
}

static void hotkey_manager_plugin_class_init(HotkeyManagerPluginClass *klass)
{
  G_OBJECT_CLASS(klass)->dispose = hotkey_manager_plugin_dispose;
}

static void hotkey_manager_plugin_init(HotkeyManagerPlugin *self) {}

static void method_call_cb(FlMethodChannel *channel, FlMethodCall *method_call,
                           gpointer user_data)
{
  HotkeyManagerPlugin *plugin = HOTKEY_MANAGER_PLUGIN(user_data);
  hotkey_manager_plugin_handle_method_call(plugin, method_call);
}

void hotkey_manager_plugin_register_with_registrar(FlPluginRegistrar *registrar)
{
  HotkeyManagerPlugin *plugin = HOTKEY_MANAGER_PLUGIN(
      g_object_new(hotkey_manager_plugin_get_type(), nullptr));

  plugin->registrar = FL_PLUGIN_REGISTRAR(g_object_ref(registrar));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "hotkey_manager",
                            FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);
}