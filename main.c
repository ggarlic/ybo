#include "playback.h"
#include "main.h"

static GtkWidget *main_window;
static GtkWidget *play_button;
static GtkWidget *pause_button;
static GtkWidget *stop_button;
static GtkWidget *status_label;
static GtkWidget *time_label;
static GtkWidget *seek_scale;
static GtkWidget *title_label;
static GtkWidget *artist_label;

static char *current_filename = NULL;

gboolean no_seek = FALSE;


static gboolean
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  return FALSE;
}

static void
destroy(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

/* Handler for File->Open action */
static void
file_open(GtkAction *action)
{
  /* Construct a GtkFileChooser */
  GtkWidget *file_chooser = gtk_file_chooser_dialog_new(
      "Open File", GTK_WINDOW(main_window),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
      NULL);

  /* Run the dialog and if the user pressed the accept button... */
  if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT)
  {
    /* ...get the URI of the chosen file */
    char *filename;
    filename = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(file_chooser));
    /* In case we're already playing a file, simulate a press of the stop button */
    g_signal_emit_by_name(G_OBJECT(stop_button), "clicked");
    if (current_filename) g_free(current_filename);
    current_filename = filename;
    /* Load the selected file */
    if (load_file(filename))
      gtk_widget_set_sensitive(GTK_WIDGET(play_button), TRUE);
  }

  gtk_widget_destroy(file_chooser);
}

/* Handler for File->Quit action */
static void
file_quit(GtkAction *action)
{
  gtk_main_quit();
}

/* Construct and display a GtkAboutDialog */
static void
help_about(GtkAction *action)
{
  GtkWidget *about_dialog = gtk_about_dialog_new();

  gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about_dialog), "YBO Player");
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), "0.1");
  gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), "Copyright 2011, ggarlic");

  /* gtk_dialog_run returns when the user dismisses the dialog */
  gtk_dialog_run(GTK_DIALOG(about_dialog));
}

/* Handler for play button click event */
static void
play_clicked(GtkWidget *widget, gpointer data)
{
  if (current_filename)
  {
    if (play_file())
    {
      gtk_widget_set_sensitive(GTK_WIDGET(stop_button), TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(pause_button), TRUE);
    }
    else
    {
      g_print("Failed to play\n");
    }
  }
}

/* Handler for stop button click */
static void
stop_clicked(GtkWidget *widget, gpointer data)
{
  stop_playback();
  gui_status_update(STATE_STOP);
}

/* Handler for user moving seek bar */
static void
seek_value_changed(GtkRange *range, gpointer data)
{
  if (no_seek) return;
  gdouble val = gtk_range_get_value(range);

  seek_to(val);
}

/* Convenience function which looks at the current state
 * and updates the window accordingly */
void
gui_status_update(PlayerState state)
{
  switch (state) {
    case STATE_STOP:
      gtk_widget_set_sensitive(GTK_WIDGET(stop_button), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(pause_button), FALSE);
      gtk_label_set_markup(GTK_LABEL(status_label), "<b>Stopped</b>");
      gtk_range_set_value(GTK_RANGE(seek_scale), 0.0);
      gtk_label_set_text(GTK_LABEL(time_label), "--:--.--");
      break;
    case STATE_PLAY:
      gtk_widget_set_sensitive(GTK_WIDGET(stop_button), TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(pause_button), TRUE);
      gtk_label_set_markup(GTK_LABEL(status_label), "<b>Playing</b>");
      break;
    case STATE_PAUSE:
      gtk_label_set_markup(GTK_LABEL(status_label), "<b>Paused</b>");
      break;
    default:
      break;
  }
}

/* Takes time values and formats them for the time label and seek slider */
void
gui_update_time(const gchar *time, const gint64 position, const gint64 length)
{
  gtk_label_set_text(GTK_LABEL(time_label), time);
  if (length > 0) {
    no_seek = TRUE;
    gtk_range_set_value(GTK_RANGE(seek_scale), ((gdouble)position / (gdouble)length) * 100.0);
    no_seek = FALSE;
  }
}

/* Convenience function to update title and artist display */
void
gui_update_metadata(const gchar *title, const gchar *artist)
{
  gtk_label_set_text(GTK_LABEL(title_label), title);
  gtk_label_set_text(GTK_LABEL(artist_label), artist);
}

/* Convenience function to stop displaying metadata */
void
gui_clear_metadata(void)
{
  gtk_label_set_text(GTK_LABEL(title_label), "");
  gtk_label_set_text(GTK_LABEL(artist_label), "");
}

/* Actions to be used by menu items */
static GtkActionEntry mainwindow_action_entries[] = {
  { "FileMenu", "NULL", "_File" },
  {
    "OpenFile",
    GTK_STOCK_OPEN,
    "_Open",
    "<control>O",
    "Open a file for playback",
    G_CALLBACK(file_open)
  },
  {
    "QuitPlayer",
    GTK_STOCK_QUIT,
    "_Quit",
    "<control>Q",
    "Quit the music player",
    G_CALLBACK(file_quit)
  },
  { "HelpMenu", "NULL", "_Help" },
  {
    "HelpAbout",
    GTK_STOCK_ABOUT,
    "_About",
    "",
    "About the music player",
    G_CALLBACK(help_about)
  }
};


/* Builds the GUI controls which will go inside the player window
 * Returns a GtkWidget * to be added to the window. In our case,
 * this will be a GtkVBox.
 */
GtkWidget *
build_gui()
{
  GtkActionGroup *actiongroup;
  GtkUIManager *ui_manager;

  GtkWidget *main_vbox;
  GtkWidget *status_hbox;
  GtkWidget *controls_hbox;

  /* Create the action group from the definitions in the static array
   * defined above */
  actiongroup = gtk_action_group_new("MainwindowActionGroup");
  gtk_action_group_add_actions(actiongroup,
      mainwindow_action_entries, G_N_ELEMENTS(mainwindow_action_entries), NULL);

  /* Construct the UI manager */
  ui_manager = gtk_ui_manager_new();
  /* Associate the action group with the UI manager */
  gtk_ui_manager_insert_action_group(ui_manager, actiongroup, 0);
  /* Pass the XML menu structure to the UI manager */
  gtk_ui_manager_add_ui_from_string(ui_manager,
      "<ui>"
      "  <menubar name='MainMenu'>"
      "    <menu action='FileMenu'>"
      "      <menuitem action='OpenFile'/>"
      "      <separator name='fsep1'/>"
      "      <menuitem action='QuitPlayer'/>"
      "    </menu>"
      "    <menu action='HelpMenu'>"
      "      <menuitem action='HelpAbout'/>"
      "    </menu>"
      "  </menubar>"
      "</ui>",
       -1,
       NULL);

  /* Create the main GtkVBox. Everything else will go inside here */
  main_vbox = gtk_vbox_new(0, 6);

  /* The first thing to go into the VBox is the menu bar, which we
   * get by asking the UI manager for /ui/MainMenu as defined in the XML */
  gtk_box_pack_start(
      GTK_BOX(main_vbox),
      gtk_ui_manager_get_widget(ui_manager, "/ui/MainMenu"),
      FALSE, FALSE, 0);

  /* Build the labels for title and artist */
  title_label = gtk_label_new("Title");
  gtk_misc_set_alignment(GTK_MISC(title_label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(main_vbox), title_label, FALSE, FALSE, 0);

  artist_label = gtk_label_new("Artist");
  gtk_misc_set_alignment(GTK_MISC(artist_label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(main_vbox), artist_label, FALSE, FALSE, 0);


  /* Status and time labels go in their own HBox so they can appear
   * side by side in the final window */
  status_hbox = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(main_vbox), status_hbox, FALSE, FALSE, 0);

  status_label = gtk_label_new("<b>Stopped</b>");
  gtk_label_set_use_markup(GTK_LABEL(status_label), TRUE);
  gtk_misc_set_alignment(GTK_MISC(status_label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(status_hbox), status_label, TRUE, TRUE, 0);

  time_label = gtk_label_new("--:--.--");
  gtk_misc_set_alignment(GTK_MISC(time_label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(status_hbox), time_label, TRUE, TRUE, 0);


  /* The seek slider is simply a GtkHScale widget. The range is
   * a percentage of the length through the file, so that we don't
   * have to change it each time we load a file with a different length */
  seek_scale = gtk_hscale_new_with_range(0, 100, 1);
  gtk_scale_set_draw_value(GTK_SCALE(seek_scale), FALSE);
  /* Set the slider to only produce a value-changed signal when the user
   * finishes dragging, not during. */
  gtk_range_set_update_policy(GTK_RANGE(seek_scale), GTK_UPDATE_DISCONTINUOUS);
  g_signal_connect(G_OBJECT(seek_scale), "value-changed", G_CALLBACK(seek_value_changed), NULL);
  gtk_box_pack_start(GTK_BOX(main_vbox), seek_scale, FALSE, FALSE, 0);


  controls_hbox = gtk_hbox_new(TRUE, 6);
  gtk_box_pack_start_defaults(GTK_BOX(main_vbox), controls_hbox);


  play_button = gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
  gtk_widget_set_sensitive(play_button, FALSE);
  g_signal_connect(G_OBJECT(play_button), "clicked", G_CALLBACK(play_clicked), NULL);
  gtk_box_pack_start_defaults(GTK_BOX(controls_hbox), play_button);

  pause_button = gtk_button_new_from_stock(GTK_STOCK_MEDIA_PAUSE);
  gtk_widget_set_sensitive(pause_button, FALSE);
  gtk_box_pack_start_defaults(GTK_BOX(controls_hbox), pause_button);

  stop_button = gtk_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
  gtk_widget_set_sensitive(stop_button, FALSE);
  g_signal_connect(G_OBJECT(stop_button), "clicked", G_CALLBACK(stop_clicked), NULL);
  gtk_box_pack_start_defaults(GTK_BOX(controls_hbox), stop_button);

  return main_vbox;
}


int
main(int argc, char *argv[])
{
  /* Initialise GTK+ */
  gtk_init(&argc, &argv);
  /* Initialise GStreamer */
  gst_init(&argc, &argv);

  /* Create the main window */
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Connect the main window's destroy handler */
  g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(destroy), NULL);

  /* Build the GUI and add it to the main window */
  gtk_container_add(GTK_CONTAINER(main_window), build_gui());

  /* Set main window title */
  gtk_window_set_title(GTK_WINDOW(main_window), "Music Player");

  /* Make everything visible */
  gtk_widget_show_all(main_window);

  /* Start the main loop */
  gtk_main();

  return 0;
}

