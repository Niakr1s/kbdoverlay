#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <cairo.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

static GtkApplication *app;
static GList *overlay_windows = NULL;

// ----------------------------------------------------------------
// click-through
static void on_realize(GtkWidget *widget, gpointer data) {
  GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(widget));
  if (surface) {
    cairo_region_t *empty = cairo_region_create();
    gdk_surface_set_input_region(surface, empty);
    cairo_region_destroy(empty);
  }
}

// ------------------------------------------------------------------
// per-monitor
static GtkWidget *create_overlay_for_monitor(GdkMonitor *monitor) {
  GtkWidget *window = gtk_application_window_new(app);

  gtk_layer_init_for_window(GTK_WINDOW(window));
  gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
  gtk_layer_set_monitor(GTK_WINDOW(window), monitor);
  gtk_layer_set_keyboard_mode(GTK_WINDOW(window),
                              GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);

  gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, 60);

  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_window_set_default_size(GTK_WINDOW(window), 200, 60);

  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_string(
      css, "window { background-color: transparent; }");
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkWidget *label = gtk_label_new("");
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  gtk_window_set_child(GTK_WINDOW(window), label);

  g_signal_connect(window, "realize", G_CALLBACK(on_realize), NULL);

  gtk_window_present(GTK_WINDOW(window));
  gtk_widget_set_visible(window, FALSE);
  return window;
}

// -------------------------------------------------------------- public helpers
void overlay_set_text(const char *markup) {
  for (GList *l = overlay_windows; l; l = l->next) {
    GtkWidget *label = gtk_window_get_child(GTK_WINDOW(l->data));
    if (label)
      gtk_label_set_markup(GTK_LABEL(label), markup ? markup : "");
  }
}

void overlay_set_visible(gboolean visible) {
  for (GList *l = overlay_windows; l; l = l->next) {
    GtkWidget *win = GTK_WIDGET(l->data);
    if (visible)
      gtk_window_present(GTK_WINDOW(win));
    else
      gtk_widget_set_visible(win, FALSE);
  }
}

void overlay_toggle(void) {
  gboolean any_visible = FALSE;
  for (GList *l = overlay_windows; l; l = l->next) {
    if (gtk_widget_get_visible(GTK_WIDGET(l->data))) {
      any_visible = TRUE;
      break;
    }
  }
  overlay_set_visible(!any_visible);
}

// ------------------------------------------------------------------ actions
static void on_toggle_action(GSimpleAction *action, GVariant *parameter,
                             gpointer user_data) {
  overlay_toggle();
}

static void on_show_action(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data) {
  overlay_set_visible(TRUE);
}

static void on_hide_action(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data) {
  overlay_set_visible(FALSE);
}

static void on_set_text_action(GSimpleAction *action, GVariant *parameter,
                               gpointer user_data) {
  const gchar *text = g_variant_get_string(parameter, NULL);
  overlay_set_text(text);
}

// ------------------------------------------------------------------ monitors
static void reconcile_monitors(void) {
  GdkDisplay *display = gdk_display_get_default();
  if (!display)
    return;

  GListModel *monitors = gdk_display_get_monitors(display);
  guint n = g_list_model_get_n_items(monitors);

  if (overlay_windows) {
    for (GList *l = overlay_windows; l; l = l->next) {
      gtk_window_destroy(GTK_WINDOW(l->data));
    }
    g_list_free(overlay_windows);
    overlay_windows = NULL;
  }

  for (guint i = 0; i < n; i++) {
    GdkMonitor *monitor = GDK_MONITOR(g_list_model_get_item(monitors, i));
    if (monitor) {
      GtkWidget *win = create_overlay_for_monitor(monitor);
      overlay_windows = g_list_append(overlay_windows, win);
      g_object_unref(monitor);
    }
  }
}

static void on_monitors_changed(GListModel *monitors, guint position,
                                guint removed, guint added,
                                gpointer user_data) {
  reconcile_monitors();
}

static void on_activate(GtkApplication *application, gpointer user_data) {
  GdkDisplay *display = gdk_display_get_default();
  if (display) {
    GListModel *monitors = gdk_display_get_monitors(display);
    g_signal_connect(monitors, "items-changed",
                     G_CALLBACK(on_monitors_changed), NULL);
  }

  // Register "toggle" (no argument)
  GSimpleAction *a_toggle = g_simple_action_new("toggle", NULL);
  g_signal_connect(a_toggle, "activate", G_CALLBACK(on_toggle_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(application), G_ACTION(a_toggle));
  g_object_unref(a_toggle);

  // Register "show" (no argument)
  GSimpleAction *a_show = g_simple_action_new("show", NULL);
  g_signal_connect(a_show, "activate", G_CALLBACK(on_show_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(application), G_ACTION(a_show));
  g_object_unref(a_show);

  // Register "hide" (no argument)
  GSimpleAction *a_hide = g_simple_action_new("hide", NULL);
  g_signal_connect(a_hide, "activate", G_CALLBACK(on_hide_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(application), G_ACTION(a_hide));
  g_object_unref(a_hide);

  // Register "set-text" (takes a string)
  GSimpleAction *a_text =
      g_simple_action_new("set-text", G_VARIANT_TYPE_STRING);
  g_signal_connect(a_text, "activate", G_CALLBACK(on_set_text_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(application), G_ACTION(a_text));
  g_object_unref(a_text);

  reconcile_monitors();
}

const char *APP_ID = "my.kbd.overlay";

static void run_gapplication(const char *action, const char *arg) {
  char cmd[4096];

  if (arg) {
    // Wrap the argument in GVariant string syntax: "..."
    // and escape any embedded double quotes for the shell.
    char escaped[2048];
    size_t j = 0;
    for (size_t i = 0; arg[i] && j < sizeof(escaped) - 2; i++) {
      if (arg[i] == '"' || arg[i] == '\\')
        escaped[j++] = '\\';
      escaped[j++] = arg[i];
    }
    escaped[j] = '\0';

    snprintf(cmd, sizeof(cmd),
             "gapplication action %s %s '\"<span>%s</span>\"'", APP_ID, action,
             escaped);
  } else {
    snprintf(cmd, sizeof(cmd), "gapplication action %s %s", APP_ID, action);
  }

  int rc = system(cmd);
  if (rc != 0)
    fprintf(stderr, "overlay_cli: command failed: %s\n", cmd);
}

void overlay_toggle_cli(void) { run_gapplication("toggle", NULL); }
void overlay_show_cli(void) { run_gapplication("show", NULL); }
void overlay_hide_cli(void) { run_gapplication("hide", NULL); }

void overlay_set_text_cli(const char *text) {
  run_gapplication("set-text", text ? text : "");
}

int run_overlay() {
  app = gtk_application_new(APP_ID, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

  int status = g_application_run(G_APPLICATION(app), 0, NULL);
  g_object_unref(app);
  return status;
}
