/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  main file
 *
 *        Version:  1.0
 *        Created:  10/24/2011 10:09:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yang bo (ggarlic), yangbo.ggarlic@gmail.com
 *        Company:  
 *
 * =====================================================================================
 */
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

static void destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}  

GtkWidget *build_gui()
{
   GtkWidget *main_vbox;
   GtkWidget *status_hbox;
   GtkWidget *controls_hbox;

   main_vbox = gtk_vbox_new(0, 6);

   //title
   title_label = gtk_label_new("Title");
   gtk_misc_set_alignment(GTK_MISC(title_label), 0.0, 0.5);
   gtk_box_pack_start(GTK_BOX(main_vbox), title_label, FALSE, FALSE, 0);

   //artist
   artist_label = gtk_label_new("Artist");
   gtk_misc_set_alignment(GTK_MISC(artist_label), 0.0, 0.5);
   gtk_box_pack_start(GTK_BOX(main_vbox), artist_label, FALSE, FALSE, 0);

   //pack of status and time
   status_hbox = gtk_hbox_new(TRUE, 0);
   gtk_box_pack_start(GTK_BOX(main_vbox), status_hbox, FALSE, FALSE, 0);
   
   //status
   status_label = gtk_label_new("<b>Stopped</b>");
   gtk_label_set_use_markup(GTK_LABEL(status_label), TRUE);
   gtk_misc_set_alignment(GTK_BOX(status_hbox), status_label, TRUE, TREUE, 0);

   //time
   time_label = gtk_label_new("--:--.--");
   gtk_misc_set_alignment(GTK_MISC(time_label), 1.0, 0.5);
   gtk_box_pack_start(GTK_BOX(status_hbox), time_label, TRUE, TRUE, 0);

   //seek scale
   seek_scale = gtk_hscale_new_with_range(0, 100, 1);
   gtk_scale_set_draw_value(GTK_SCALE(seek_scale), FALSE);
   gtk_range_set_update_policy(GTK_RANGE(seek_scale), GTK_UPDATE_DISCONTINUOUS);
   gtk_box_pack_start(GTK_BOX(main_vbox), seek_scale, FALSE, FALSE, 0);

   //pack of buttons
   controls_hbox = gtk_hbox_new(TRUE, 6);
   gtk_box_pack_start_defaults(GTK_BOX(main_vbox), controls_hbox);

   //play button
   play_button = gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
   gtk_widget_set_sensitive(play_button, FLASE);
   gtk_box_pack_start_defaults(GTK_BOX(controls_hbox), play_button);


}

int main(int argc, char *argv[])
{ 
    gtk_init(&argc, &argv);

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(main_window), "YBO");

    g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(destroy), NULL);

    gtk_widget_show_all(GTK_WIDGET(main_window));
    
    gtk_main();
    return 0;
}
