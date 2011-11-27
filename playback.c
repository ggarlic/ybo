#include "main.h"
#include "playback.h"

static GstElement *play = NULL;
static guint timeout_source = 0;

/* Callback function invoked when a message arrives on the playback
 * pipeline's bus. */
static gboolean
bus_callback (GstBus *bus, GstMessage *message, gpointer data)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR: {
      /* An error has occurred.
       * A real application would probably need to handle this more
       * intelligently than just quitting. */
      GError *err;
      gchar *debug;

      gst_message_parse_error(message, &err, &debug);
      g_print("Error: %s\n", err->message);
      g_error_free(err);
      g_free(debug);

      gtk_main_quit();
      break;
    }
  
    case GST_MESSAGE_EOS:
      /* The pipeline has reached the end of the stream. */
      g_print("End of stream\n");
      stop_playback();
      gui_status_update(STATE_STOP);
      break;

    case GST_MESSAGE_TAG: {
      /* The stream discovered new tags. */
      GstTagList *tags;
      gchar *title  = "";
      gchar *artist = "";
      /* Extract from the message the GstTagList.
       * This generates a copy, so we must remember to free it.*/
      gst_message_parse_tag(message, &tags);
      /* Extract the title and artist tags - if they exist */
      if (gst_tag_list_get_string(tags, GST_TAG_TITLE, &title)
	  && gst_tag_list_get_string(tags, GST_TAG_ARTIST, &artist))
	gui_update_metadata(title, artist);
      /* Free the tag list */
      gst_tag_list_free(tags);
      break;
    }

    default:
      /* Another message occurred which we are not interested in handling. */
      break;
  }

  /* We have handled this message, so indicate that it should be removed from
   * the queue.*/
  return TRUE;
}


/* This function is called every 200 milliseconds.
 * It retrieves the pipeline's playback position and updates the GUI with it. */
static gboolean
update_time_callback(GstElement *pipeline)
{
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 position;
  gint64 length;
  gchar time_buffer[25];

  if (gst_element_query_position(pipeline, &fmt, &position) && gst_element_query_duration(pipeline, &fmt, &length)) {
    g_snprintf(time_buffer, 24, "%u:%02u.%02u", GST_TIME_ARGS(position));
    gui_update_time(time_buffer, position, length);
  }

  return TRUE;
}


/* Given a URI, constructs a pipeline to play it.
 * Uses GStreamer's supplied 'playbin' element as an automatic solution */
static gboolean
build_gstreamer_pipeline(const gchar *uri)
{
  /* Destroy the pipeline if there is one already to avoid leaks */
  if (play) {
    gst_element_set_state(play, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(play));
    play = NULL;
  }

  /* Create and initialise a playbin element */
  play = gst_element_factory_make("playbin", "play");
  if (!play) return FALSE;
  g_object_set(G_OBJECT(play), "uri", uri, NULL);

  /* Connect the message bus callback to the playbin */
  gst_bus_add_watch(gst_pipeline_get_bus(GST_PIPELINE(play)), bus_callback, NULL);

  return TRUE;
}


/* Attempt to load a file */
gboolean
load_file(const gchar *uri) {
  if (build_gstreamer_pipeline(uri))
    return TRUE;

  return FALSE;
}


/* Attempt to play the loaded file */
gboolean
play_file() {

  if (play) {
    /* Start playing */
    gst_element_set_state(play, GST_STATE_PLAYING);
    gui_status_update(STATE_PLAY);
    /* Connect a callback to trigger every 200 milliseconds to
     * update the GUI with the playback progress. We remember
     * the ID of this source so that we can remove it when we stop
     * playing */
    timeout_source = g_timeout_add(200, (GSourceFunc)update_time_callback, play);
    return TRUE;
  }

  return FALSE;
}


/* Stop playing a file, if we're playing one. */
void
stop_playback()
{
  /* Remove the timeout function */
  if (timeout_source) g_source_remove(timeout_source);
  timeout_source = 0;

  /* Stop playback and delete the pipeline */
  if (play) {
    gst_element_set_state(play, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(play));
    play = NULL;
  }

  /* Update the GUI */
  gui_status_update(STATE_STOP);
}


/* Attempt to seek to the given percentage through the file */
void
seek_to(gdouble percentage)
{
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 length;

  /* If it seems safe to attempt a seek... */
  if (play && gst_element_query_duration(play, &fmt, &length)) {
    /* ...calculate where to seek to */
    gint64 target = ((gdouble)length * (percentage / 100.0));

    /* ...and attempt the seek */
    if (!gst_element_seek(play, 1.0, GST_FORMAT_TIME,
	  GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
	  target, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
      g_print("Failed to seek to desired position\n");
  }
}

