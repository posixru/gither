#include <gtk/gtk.h>
#include "callbacks.h"
#include "ui.h"

enum
    {
      TARGET_STRING,
      TARGET_URL
    };

static GtkTargetEntry targetentries[] =
    {
      { "STRING",        0, TARGET_STRING },
      { "text/plain",    0, TARGET_STRING },
      { "text/uri-list", 0, TARGET_URL }
      
    };


int
main (int argc, char *argv[])
{
GtkWidget *iconview,*new,*open,*save,*quit,*revert;
GtkWidget *help,*about,*statusbar,*toolsave,*toolrevert;
GtkBuilder *builder;
GError *error=NULL;
GtkTreeModel *model;

gtk_disable_setlocale ();
gtk_init (&argc, &argv);

builder=gtk_builder_new();
gtk_builder_add_from_string(builder,(const char *)gither_ui,-1,&error);

if(error){
g_error("Interface Load Error:%s",error->message);
g_error_free(error);
error=NULL;
g_object_unref(builder);
gtk_main_quit();
}

gtk_builder_connect_signals(builder,NULL);

mwindow=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
iconview=GTK_WIDGET(gtk_builder_get_object(builder,"iconview1"));
new=GTK_WIDGET(gtk_builder_get_object(builder,"menunew"));
open=GTK_WIDGET(gtk_builder_get_object(builder,"menuopen"));
save=GTK_WIDGET(gtk_builder_get_object(builder,"menusave"));
/*saveas=GTK_WIDGET(gtk_builder_get_object(builder,"menusaveas"));*/
quit=GTK_WIDGET(gtk_builder_get_object(builder,"menuquit"));
revert=GTK_WIDGET(gtk_builder_get_object(builder,"menurevert"));
help=GTK_WIDGET(gtk_builder_get_object(builder,"menuhelp"));
about=GTK_WIDGET(gtk_builder_get_object(builder,"menuabout"));
statusbar=GTK_WIDGET(gtk_builder_get_object(builder,"statusbar1"));
toolsave=GTK_WIDGET(gtk_builder_get_object(builder,"toolsave"));
toolrevert=GTK_WIDGET(gtk_builder_get_object(builder,"toolrevert"));

g_object_unref(builder);

gtk_widget_set_sensitive(open,FALSE);
gtk_widget_set_sensitive(save,FALSE);
/*gtk_widget_set_sensitive(saveas,FALSE);*/
gtk_widget_set_sensitive(revert,FALSE);
gtk_widget_set_sensitive(toolsave,FALSE);
gtk_widget_set_sensitive(toolrevert,FALSE);

gtk_widget_set_size_request(mwindow,550,410);
gtk_window_set_title(GTK_WINDOW(mwindow),"gither");

gtk_icon_view_set_item_width(GTK_ICON_VIEW(iconview),140);

gtk_icon_view_set_text_column(GTK_ICON_VIEW(iconview),1);
gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(iconview),0);


gtk_drag_dest_set(iconview, GTK_DEST_DEFAULT_ALL, targetentries, 3,GDK_ACTION_COPY);
gtk_icon_view_enable_model_drag_dest(GTK_ICON_VIEW(iconview),targetentries,3,GDK_ACTION_COPY);

model=create_model();

gtk_icon_view_set_model(GTK_ICON_VIEW(iconview),model);

g_object_unref(model);

g_object_set_data(G_OBJECT(mwindow),"iconview",(gpointer)iconview);
g_object_set_data(G_OBJECT(mwindow),"statusbar",(gpointer)statusbar);
g_object_set_data(G_OBJECT(mwindow),"open",(gpointer)open);
g_object_set_data(G_OBJECT(mwindow),"save",(gpointer)save);
/*g_object_set_data(G_OBJECT(mwindow),"saveas",(gpointer)saveas);*/
g_object_set_data(G_OBJECT(mwindow),"revert",(gpointer)revert);
g_object_set_data(G_OBJECT(mwindow),"toolsave",(gpointer)toolsave);
g_object_set_data(G_OBJECT(mwindow),"toolrevert",(gpointer)toolrevert);

g_signal_connect(G_OBJECT(new),"activate",
                 G_CALLBACK(on_new_activate),NULL);

g_signal_connect(G_OBJECT(open),"activate",
                 G_CALLBACK(on_open_activate),NULL);

g_signal_connect(G_OBJECT(save),"activate",
                 G_CALLBACK(on_save_activate),NULL);

g_signal_connect(G_OBJECT(quit),"activate",
                 G_CALLBACK(on_quit_activate),NULL);

g_signal_connect(G_OBJECT(revert),"activate",
                 G_CALLBACK(on_revert_activate),(gpointer) iconview);

g_signal_connect(G_OBJECT(help),"activate",
                 G_CALLBACK(on_help_activate),NULL);

g_signal_connect(G_OBJECT(about),"activate",
                 G_CALLBACK(on_about_activate),NULL);

g_signal_connect(G_OBJECT(toolsave),"clicked",
                 G_CALLBACK(on_toolsave_clicked),(gpointer) save);

g_signal_connect(G_OBJECT(toolrevert),"clicked",
                 G_CALLBACK(on_toolrevert_clicked),(gpointer) revert);

g_signal_connect(G_OBJECT(iconview), "drag_data_received",
                     G_CALLBACK(add_dropped_icon),NULL);

g_signal_connect(G_OBJECT (mwindow), "delete_event",
  	 G_CALLBACK (delete_event), NULL);

g_signal_connect(G_OBJECT(mwindow),"destroy",
                 G_CALLBACK(gtk_main_quit),NULL);

gtk_widget_show_all(mwindow);

gtk_main ();


  return 0;
}
