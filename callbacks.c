#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define GETTEXT_PACKAGE "gtk20"
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <gio/gio.h>
#include "ui.h"
#include "callbacks.h"

typedef struct
{
char *size;
char *context;
char *type;
char *directory;
}
IconProperties;

enum
{
C_PIXBUF,
C_NAME,
C_SNAME,
C_SOURCE,
C_NEW,
C_USED
};

static GSList *flist=NULL,*llist=NULL,*theme_list=NULL;
char *savedir=NULL,*current_theme=NULL,*backup_theme=NULL,*current_context=NULL;
gboolean method1=FALSE;
int acount=0;

GtkTreeModel
*create_model(void)
{
GtkListStore *store;
store = gtk_list_store_new (6,GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN);
gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),C_NAME,GTK_SORT_ASCENDING);
gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (store),default_sort_func,NULL, NULL);
return GTK_TREE_MODEL(store);
}

GtkTreeModel
*create_model_open(void)
{
GtkListStore *store;
store = gtk_list_store_new(2,GDK_TYPE_PIXBUF,G_TYPE_STRING); 
return GTK_TREE_MODEL(store);
}

void 
on_new_activate(GtkMenuItem *menuitem,gpointer user_data)
{
GtkWidget *window1,*radio1,*button1;
GtkBuilder *builder;

builder=gtk_builder_new();
gtk_builder_add_from_string(builder,(const char *)gither_new_ui,-1,NULL);

gtk_builder_connect_signals(builder,NULL);

window1=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
radio1=GTK_WIDGET(gtk_builder_get_object(builder,"radiobutton1"));
button1=GTK_WIDGET(gtk_builder_get_object(builder,"button1"));

g_object_unref(builder);

gtk_window_set_transient_for(GTK_WINDOW(window1),GTK_WINDOW(mwindow));
gtk_window_set_position(GTK_WINDOW(window1),GTK_WIN_POS_CENTER_ON_PARENT);
gtk_window_set_title(GTK_WINDOW(window1),_("New"));

g_signal_connect(G_OBJECT(button1),"clicked",
                 G_CALLBACK(initialize_method),(gpointer) radio1);

g_signal_connect_swapped(G_OBJECT(button1),"clicked",
                         G_CALLBACK(gtk_widget_destroy),(gpointer) window1);

g_signal_connect(G_OBJECT(radio1),"toggled",
                 G_CALLBACK(set_method),NULL);

gtk_widget_show_all(window1);
set_widget_state("open",TRUE);
}

void 
on_open_activate(GtkMenuItem *menuitem,gpointer user_data)
{
GtkWidget *dialog,*combobox1,*combobox2,*ok_button;
GtkBuilder *builder;
GtkTreeModel *model;
GtkCellRenderer *renderer;
GdkCursor *cursor;
GdkDisplay *display;

display=gdk_display_get_default();
cursor=gdk_cursor_new_from_name(display,"watch");
gdk_window_set_cursor(mwindow->window,cursor);


builder=gtk_builder_new();
gtk_builder_add_from_string(builder,(const char *)gither_open_ui,-1,NULL);

gtk_builder_connect_signals(builder,NULL);

dialog=GTK_WIDGET(gtk_builder_get_object(builder,"dialog1"));
combobox1=GTK_WIDGET(gtk_builder_get_object(builder,"combobox1"));
combobox2=GTK_WIDGET(gtk_builder_get_object(builder,"combobox2"));
ok_button=GTK_WIDGET(gtk_builder_get_object(builder,"button2"));
g_object_unref(builder);
gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(mwindow));
gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER_ON_PARENT);
gtk_window_set_title(GTK_WINDOW(dialog),_("Open"));
model=create_model_open();
gtk_combo_box_set_model(GTK_COMBO_BOX(combobox1),model);
fill_model_open(combobox1,combobox2,model);

gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combobox1),combo_separator,NULL,NULL);

g_object_unref(model);

gtk_cell_layout_clear(GTK_CELL_LAYOUT (combobox1));
renderer = gtk_cell_renderer_pixbuf_new ();
gtk_cell_renderer_set_fixed_size(GTK_CELL_RENDERER(renderer),32,32);
gtk_cell_renderer_set_padding(GTK_CELL_RENDERER(renderer),0,0);
gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox1), renderer, TRUE);
gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox1), renderer,
  		        "pixbuf", 0,NULL);
renderer = gtk_cell_renderer_text_new ();
gtk_cell_renderer_set_fixed_size(GTK_CELL_RENDERER(renderer),100,-1);
gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox1), renderer, TRUE);
gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox1), renderer,
			        "text", 1,NULL);

g_signal_connect(G_OBJECT(combobox1),"changed",
                 G_CALLBACK(get_selected_theme),(gpointer) combobox2);

g_signal_connect(G_OBJECT(ok_button),"clicked",
                 G_CALLBACK(open_icon_theme),(gpointer) combobox2);

g_signal_connect_swapped(G_OBJECT(ok_button),"clicked",
                         G_CALLBACK(gtk_widget_destroy),(gpointer) dialog);

gtk_widget_show_all(dialog);
gdk_cursor_unref(cursor);
gdk_window_set_cursor(mwindow->window,NULL);
}

void 
fill_model_open(GtkWidget *widget1,GtkWidget *widget2,GtkTreeModel *model)
{
const char *homedir=g_getenv ("HOME");
char *readname,*icondir,*filename,*themefile,*sample_icon=NULL;
static GdkPixbuf *pixbuf=NULL;
GtkTreeIter iter;
GDir *dir;
GError *error=NULL;
int ok=0;
gboolean state=FALSE;

if(theme_list){
g_slist_foreach(theme_list,(GFunc)g_free,NULL);
g_slist_free(theme_list);
theme_list=NULL;
	      }

state=strcmp("root",g_get_user_name());

if(state){

if (!homedir) homedir = g_get_home_dir ();
icondir=g_build_path(G_DIR_SEPARATOR_S,homedir,".icons",NULL);
dir=g_dir_open(icondir,0,&error);
if(error) return _display_message("Directory Open Error:",error,icondir);

readname=g_new(char,1);
while(readname){
while(gtk_events_pending()) gtk_main_iteration();
readname=(char *)g_dir_read_name(dir);
if(!readname) break;
filename=g_build_path(G_DIR_SEPARATOR_S,icondir,readname,NULL);
if(g_file_test(filename,G_FILE_TEST_IS_DIR)){
themefile=g_build_path(G_DIR_SEPARATOR_S,filename,"index.theme",NULL);
if(g_file_test(themefile,G_FILE_TEST_EXISTS))    {
sample_icon=check_icon_theme(filename,&ok);
if(ok){
theme_list=g_slist_append(theme_list,g_strdup(filename));
pixbuf=gdk_pixbuf_new_from_file_at_size(sample_icon,32,32,&error);
if(pixbuf){
gtk_list_store_append (GTK_LIST_STORE(model), &iter);
gtk_list_store_set (GTK_LIST_STORE(model), &iter,0,pixbuf,1,readname,-1);
	  }
else{
g_print("Pixbuf Load Error:%s\n",error->message);
g_error_free(error);
error=NULL;
    }
g_object_unref(pixbuf);
      }
g_free(sample_icon);

						}
g_free(themefile);
					    }
g_free(filename);

	       }
g_dir_close(dir);
g_free(icondir);

	 }

state=(!method1 && !strcmp("root",g_get_user_name()));

if(method1 || state){
icondir=g_strdup("/usr/share/icons");
dir=g_dir_open(icondir,0,&error);
if(error) return _display_message("Directory Open Error:",error,icondir);

readname=g_new(char,1);
while(readname){
while(gtk_events_pending()) gtk_main_iteration();
readname=(char *)g_dir_read_name(dir);
if(!readname) break;
filename=g_build_path(G_DIR_SEPARATOR_S,icondir,readname,NULL);
if(g_file_test(filename,G_FILE_TEST_IS_DIR)){
themefile=g_build_path(G_DIR_SEPARATOR_S,filename,"index.theme",NULL);
if(g_file_test(themefile,G_FILE_TEST_EXISTS))    {
sample_icon=check_icon_theme(filename,&ok);
if(ok){
theme_list=g_slist_append(theme_list,g_strdup(filename));
pixbuf=gdk_pixbuf_new_from_file_at_size(sample_icon,32,32,&error);
if(pixbuf){
gtk_list_store_append (GTK_LIST_STORE(model), &iter);
gtk_list_store_set (GTK_LIST_STORE(model), &iter,0,pixbuf,1,readname,-1);
	  }
else{
g_print("Pixbuf Load Error:%s\n",error->message);
g_error_free(error);
error=NULL;
    }
g_object_unref(pixbuf);
      }
g_free(sample_icon);

						}
g_free(themefile);
					    }
g_free(filename);

	       }
g_dir_close(dir);
g_free(icondir);

		    }

gtk_list_store_append (GTK_LIST_STORE(model), &iter);
gtk_list_store_set (GTK_LIST_STORE(model), &iter,0,NULL,1,"separator",-1);

gtk_list_store_append (GTK_LIST_STORE(model), &iter);
gtk_list_store_set (GTK_LIST_STORE(model), &iter,0,NULL,1,"other...",-1);

if(current_theme){
gtk_combo_box_set_active(GTK_COMBO_BOX(widget1),set_active_theme());
get_selected_theme(widget1,widget2);
		 }
}

int 
set_active_theme(void)
{
int i,ret=0,ok=0;
for(i=0;i<g_slist_length(theme_list);i++){
if(ret) break;
if(!strcmp(current_theme,g_slist_nth_data(theme_list,i))){
ret=i;
ok=1;
							 }
					 }
if(ok) return ret;
else return -1;
}

void 
open_icon_theme(GtkButton *button,gpointer user_data)
{
GtkWidget *combobox=GTK_WIDGET(user_data);
GtkTreeIter iter;
GtkTreeModel *model;
GArray *table=NULL;
IconProperties properties;
gchar *context_name,*fullpath;
int i,j,ok=0;
const char *valid_size[2]={"48","128"};


if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combobox),&iter))
{
model=gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
gtk_tree_model_get (model, &iter,0,&context_name,-1);
table=g_array_sized_new (FALSE, FALSE, sizeof (IconProperties), 1);
if(read_icon_theme(table,(const char *)current_theme)){
for(i=0;i<table->len;i++){
if(ok) break;
properties=g_array_index(table,IconProperties,i);
if(!strcmp(context_name,properties.context)){
for(j=0;j<2;j++){
if(!strcmp(valid_size[j],properties.size)){
fullpath=g_build_path(G_DIR_SEPARATOR_S,current_theme,properties.directory,NULL);
ok=1;
					  } 
		}
					    }	

					    
			 }

if(ok){
open_icons(fullpath);
acount=0;
g_free(fullpath);
current_context=g_strdup(context_name);
      }
else show_message("Theme not have a valid icon size\nCheck \"index.theme\" file",GTK_MESSAGE_WARNING);
 						      }
g_array_free(table,TRUE); 
g_free(context_name);
}

}

void 
open_icons(char *fullpath)
{
GtkWidget *iconview;
GtkTreeModel *model;
GtkTreeIter iter;
static GdkPixbuf *pixbuf=NULL;
GDir *dir;
GError *error=NULL;
char *readname,*filename,*bname,*sname,*fname,*lname,*wtitle;
gboolean is_symlink=FALSE;

dir=g_dir_open(fullpath,0,&error);
if(error) return _display_message("Directory Open Error",error,fullpath);

iconview=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),"iconview");
model=gtk_icon_view_get_model(GTK_ICON_VIEW(iconview));
gtk_list_store_clear(GTK_LIST_STORE(model));

if(flist){
g_slist_foreach(flist,(GFunc)g_free,NULL);
g_slist_free(flist);
flist=NULL;
         }
if(llist){
g_slist_foreach(llist,(GFunc)g_free,NULL);
g_slist_free(llist);
llist=NULL;
         }

readname=g_new(char,1);
while(readname){
readname=(char *)g_dir_read_name(dir);
if(!readname) break;
if(g_str_has_suffix(readname,".png") || g_str_has_suffix(readname,".svg")){
filename=g_build_path(G_DIR_SEPARATOR_S,fullpath,readname,NULL);
is_symlink=g_file_test(filename,G_FILE_TEST_IS_SYMLINK);
if(!is_symlink)
{
pixbuf=gdk_pixbuf_new_from_file_at_size(filename,48,48,&error);
if(!error){
bname=g_path_get_basename(g_filename_to_utf8 (filename, -1, NULL, NULL, NULL));
sname=remove_suffix(bname);
gtk_list_store_append (GTK_LIST_STORE(model), &iter);
gtk_list_store_set (GTK_LIST_STORE(model), &iter, 
                                  C_PIXBUF, pixbuf, 
                                  C_NAME, bname,
				  C_SNAME,sname,
				  C_SOURCE, filename,		
                                  C_NEW, NULL,
                                  C_USED, FALSE,
                                 -1);
g_free(bname);
g_free(sname);
g_object_unref(pixbuf);

          } 
else _display_message("Pixbuf Load Error",error,NULL);
}	       
else{
fname=g_file_read_link(filename,NULL);
fname=remove_suffix(g_path_get_basename(fname));
lname=remove_suffix(g_path_get_basename(filename));
flist=g_slist_append(flist,g_strdup(fname));
llist=g_slist_append(llist,g_strdup(lname));
g_free(fname);
g_free(lname);
    }	
g_free(filename);
									  }
	       }
g_dir_close(dir);


bname=g_path_get_basename(current_theme);
wtitle=g_strconcat("gither","-(",bname,")",NULL);
gtk_window_set_title(GTK_WINDOW(mwindow),wtitle);
g_free(bname);
g_free(wtitle);

}

char 
*get_other_theme(GtkWidget *widget)
{
GtkWidget *dialog,*cellview;
GtkTreeModel *model;
GtkTreeIter iter;
GdkPixbuf *pixbuf;
char *filename=NULL,*bname;
GtkFileFilter *filter;
gtk_widget_set_sensitive(widget,FALSE);
dialog = gtk_file_chooser_dialog_new (_("Open"),
				      GTK_WINDOW(mwindow),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

filter = gtk_file_filter_new ();
gtk_file_filter_set_name(filter,"theme");        
gtk_file_filter_add_pattern (filter, "*.theme");
gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);
if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
filename=g_path_get_dirname(filename);
model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
cellview = gtk_cell_view_new ();
pixbuf = gtk_widget_render_icon (cellview, GTK_STOCK_DIRECTORY,
				GTK_ICON_SIZE_DND, NULL);
gtk_widget_destroy(cellview);
bname=g_path_get_basename(filename);
gtk_list_store_prepend (GTK_LIST_STORE(model), &iter);
gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0,pixbuf,1,bname,-1);
g_free(bname);
g_object_unref(pixbuf);
gtk_widget_destroy(dialog);
gtk_widget_set_sensitive(widget,TRUE);
gtk_combo_box_set_active(GTK_COMBO_BOX(widget),0); 
return filename;
  }
gtk_widget_destroy(dialog);
gtk_widget_set_sensitive(widget,TRUE);
gtk_combo_box_set_active(GTK_COMBO_BOX(widget),-1); 
return filename;
}

void 
get_selected_theme(GtkWidget *widget,gpointer user_data)
{
GtkWidget *combobox=GTK_WIDGET(user_data);
GtkCellRenderer *renderer;
GArray *table=NULL;
int result=0,i=0,ai=0;
IconProperties properties;
GtkListStore *store=NULL;
GtkTreeIter iter;
GSList *list=NULL;


ai=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
if(ai<0) return;

if(ai>g_slist_length(theme_list))
current_theme = get_other_theme(widget);

else
current_theme = g_strdup(g_slist_nth_data(theme_list,ai));

if(!current_theme) return;

backup_theme=g_strdup(current_theme);

table=g_array_sized_new (FALSE, FALSE, sizeof (IconProperties), 1);

result=read_icon_theme(table,(const char *)current_theme);
if(result){
gtk_widget_set_sensitive(combobox,TRUE);
store=gtk_list_store_new(1,G_TYPE_STRING);
gtk_combo_box_set_model(GTK_COMBO_BOX(combobox),GTK_TREE_MODEL(store));
g_object_unref(store);
renderer = gtk_cell_renderer_text_new ();
gtk_cell_layout_clear(GTK_CELL_LAYOUT (combobox));
gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox), renderer, TRUE);
gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,
				"text", 0,
				NULL);
for(i=0;i<table->len;i++){
properties=g_array_index(table,IconProperties,i);
list=g_slist_append(list,g_strdup(properties.context));
if(!sort_context_list(list,properties.context)){
gtk_list_store_append (store, &iter);
gtk_list_store_set (store, &iter, 0,properties.context,-1); 
				               }
			 }


	  }
g_array_free(table,TRUE); 
if(list){
g_slist_foreach(list,(GFunc)g_free,NULL);
g_slist_free(list);
list=NULL;
        }
}

int 
sort_context_list(GSList *list,const char *data)
{
int i,count=0;
for(i=0;i<g_slist_length(list);i++){
if(!strcmp(g_slist_nth_data(list,i),data)) count++;
				   }
if(count>1) return 1;
else return 0;

}

char 
*check_icon_theme(const char *icon_theme,int *is_valid)
{
char *filename,*copied,*content,**icondirs=NULL,**contents=NULL,**compared=NULL;
int i=0,l=0,ok=0,j=0,is=0;
GError *error=NULL;
IconProperties properties;
GArray *table = NULL;
const char *valid_size[3]={"32","48","128"};
char *display_icon=NULL,*testfile;

filename=g_build_path(G_DIR_SEPARATOR_S,icon_theme,"index.theme",NULL);



if(!g_file_get_contents(filename,&content,NULL,&error))
{
g_print("File Open Error:%s\n",error->message);
g_error_free(error);
error=NULL;
g_free(filename);
*is_valid=0;
return display_icon;
}

g_free(filename);

contents=g_strsplit(content,"\n",0);
g_free(content);

table=g_array_sized_new (FALSE, FALSE, sizeof (IconProperties), 1);

while(contents[l]!=0){
if(ok) break;
if(strstr(contents[l],"Directories="))  {
is=1;
copied=g_strdup(contents[l]+12);
icondirs=g_strsplit(copied,",",0);
g_free(copied);
compared=g_strdupv(icondirs);
while(compared[j]!=0){
compared[j]=g_strconcat("[",compared[j],"]",NULL);
j++;
		     }
ok=1;

					}
l++;
		     }
l--;

if(!is){
g_strfreev(icondirs);
g_strfreev(contents); 
g_strfreev(compared); 
*is_valid=0;
return display_icon;
       }

while(contents[l]!=0){
ok=0;
j=0;
i=l;
while(compared[j]!=0) {
if(ok) break;
if(!strcmp(compared[j],contents[i])){
properties.directory=g_strdup(icondirs[j]);
while(!ok){
i++;
if(contents[i][0]=='[' || contents[i][0]=='\0') ok=1;
else if(g_strrstr_len(contents[i],5,"Size=")) properties.size=g_strdup(contents[i]+5);
else if(g_strrstr_len(contents[i],8,"Context=")) properties.context=g_strdup(contents[i]+8);
else if(g_strrstr_len(contents[i],5,"Type=")) properties.type=g_strdup(contents[i]+5);
else continue;

	  }
testfile=g_build_path(G_DIR_SEPARATOR_S,icon_theme,properties.directory,NULL);
if(g_file_test(testfile,G_FILE_TEST_EXISTS))
g_array_append_vals (table, &properties, 1);
g_free(testfile);
				    }

j++;
		      }


l++;
		     }

ok=0;

for(i=0;i<table->len;i++){
if(ok) break;
properties=g_array_index(table,IconProperties,i);
if(!strcmp("Places",properties.context)){
for(j=0;j<3;j++){
if(ok) break;
if(!strcmp(valid_size[j],properties.size)){
display_icon=g_build_path(G_DIR_SEPARATOR_S,icon_theme,properties.directory,"folder.png",NULL);
if(!g_file_test(display_icon,G_FILE_TEST_EXISTS)) 
display_icon=g_build_path(G_DIR_SEPARATOR_S,icon_theme,properties.directory,"folder.svg",NULL);
ok=1;
					  }
		}
				        }
		         }


g_array_free(table,TRUE); 
g_strfreev(icondirs);
g_strfreev(contents); 
g_strfreev(compared);
if(!g_file_test(display_icon,G_FILE_TEST_EXISTS)) *is_valid=0;
else *is_valid=1; 

return display_icon;
}


int 
read_icon_theme(GArray *array,const char *icon_theme)
{
char *filename,*copied,*content,**icondirs=NULL,**contents=NULL,**compared=NULL;
int i=0,l=0,ok=0,j=0,is=0;
GError *error=NULL;
IconProperties properties;
char *testfile;

filename=g_build_path(G_DIR_SEPARATOR_S,icon_theme,"index.theme",NULL);

if(!g_file_get_contents(filename,&content,NULL,&error)) {
_display_message("Read Error",error,filename);
return 0;
							}
g_free(filename);

contents=g_strsplit(content,"\n",0);
g_free(content);

while(contents[l]!=0){
if(ok) break;
if(strstr(contents[l],"Directories="))  {
is=1;
copied=g_strdup(contents[l]+12);
icondirs=g_strsplit(copied,",",0);
g_free(copied);
compared=g_strdupv(icondirs);
while(compared[j]!=0){
compared[j]=g_strconcat("[",compared[j],"]",NULL);
j++;
		     }
ok=1;

					}
l++;
		     }
l--;

if(!is){
g_strfreev(icondirs);
g_strfreev(contents); 
g_strfreev(compared); 
return 0;
       }

while(contents[l]!=0){
ok=0;
j=0;
i=l;
while(compared[j]!=0) {
if(ok) break;
if(!strcmp(compared[j],contents[i])){
properties.directory=g_strdup(icondirs[j]);
while(!ok){
i++;
if(contents[i][0]=='[' || contents[i][0]=='\0') ok=1;
else if(g_strrstr_len(contents[i],5,"Size=")) properties.size=g_strdup(contents[i]+5);
else if(g_strrstr_len(contents[i],8,"Context=")) properties.context=g_strdup(contents[i]+8);
else if(g_strrstr_len(contents[i],5,"Type=")) properties.type=g_strdup(contents[i]+5);
else continue;

	  }
testfile=g_build_path(G_DIR_SEPARATOR_S,icon_theme,properties.directory,NULL);
if(g_file_test(testfile,G_FILE_TEST_EXISTS))
g_array_append_vals (array, &properties, 1);
g_free(testfile);

				    }

j++;
		      }


l++;
		     }
g_strfreev(icondirs);
g_strfreev(contents); 
g_strfreev(compared); 
return 1;
}

void 
on_save_activate(GtkMenuItem *menuitem,gpointer user_data)
{
if(method1){
GtkWidget *window1,*radio1,*filechooser2,*save_button,*filechooser1;
GtkBuilder *builder;
GtkFileFilter *filter;
GtkWidget *info_bar,*label1,*label2,*vbox,*content_area;
PangoFontDescription *font_desc;

builder=gtk_builder_new();
gtk_builder_add_from_string(builder,(const char *)gither_save_ui,-1,NULL);
gtk_builder_connect_signals(builder,NULL);

window1=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
radio1=GTK_WIDGET(gtk_builder_get_object(builder,"radiobutton1"));
save_button=GTK_WIDGET(gtk_builder_get_object(builder,"button2"));
filechooser1=GTK_WIDGET(gtk_builder_get_object(builder,"filechooserbutton1"));
filechooser2=GTK_WIDGET(gtk_builder_get_object(builder,"filechooserbutton2"));
g_object_unref(builder);
gtk_window_set_transient_for(GTK_WINDOW(window1),GTK_WINDOW(mwindow));
gtk_window_set_position(GTK_WINDOW(window1),GTK_WIN_POS_CENTER_ON_PARENT);
gtk_window_set_title(GTK_WINDOW(window1),_("Save"));
filter = gtk_file_filter_new ();
gtk_file_filter_set_name(filter,"theme");        
gtk_file_filter_add_pattern (filter, "*.theme");
gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (filechooser2),filter);
info_bar = gtk_info_bar_new();
gtk_widget_show(info_bar);
content_area=gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar));
gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),GTK_MESSAGE_INFO);
vbox=gtk_vbox_new(FALSE,0);
gtk_widget_show(vbox);
label1 = gtk_label_new ("Select the \"index.theme\" file of an icon theme to see the size(s)\nFollowing notations is used: s=Scalable&f=Fixed");
gtk_widget_show(label1);
gtk_label_set_justify(GTK_LABEL(label1),GTK_JUSTIFY_CENTER);
gtk_box_pack_start(GTK_BOX(vbox),label1,FALSE,FALSE,0);
label2=gtk_label_new(NULL);
gtk_widget_show(label2);
font_desc = pango_font_description_from_string ("Bold");
gtk_widget_modify_font (label2, font_desc);
pango_font_description_free (font_desc);
gtk_box_pack_start(GTK_BOX(vbox),label2,FALSE,FALSE,0);
gtk_container_add (GTK_CONTAINER (content_area), vbox);
gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER (filechooser2),info_bar);
if(savedir) 
gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (filechooser1),savedir);
g_object_set_data(G_OBJECT(save_button),"radio1",(gpointer)radio1);

g_signal_connect(G_OBJECT(radio1),"toggled",
                 G_CALLBACK(set_save_method),filechooser2);

g_signal_connect(G_OBJECT(radio1),"toggled",
                 G_CALLBACK(set_default_theme),NULL);

g_signal_connect(G_OBJECT(filechooser1),"selection-changed",
                 G_CALLBACK(set_save_dir),NULL);

g_signal_connect(G_OBJECT(filechooser2),"update-preview",
                 G_CALLBACK(show_icon_size),(gpointer) label2);

g_signal_connect(G_OBJECT(filechooser2),"file-set",
                 G_CALLBACK(set_custom_theme),NULL);

g_signal_connect(G_OBJECT(save_button),"clicked",
                 G_CALLBACK(save_icons_one),(gpointer) window1);

gtk_widget_show_all(window1);
	   }
else save_icons_two();
}


void 
save_icons_two(void)
{
GtkWidget *iconview;
GtkTreeModel *model;
GtkTreeIter iter;
GdkPixbuf *pixbuf=NULL;
GArray *table=NULL;
IconProperties properties;
GFile *source,*dest;
GError *error=NULL;
char *new_icon,*sname,*temp,*source_icon,*filename;
gboolean icon_used=FALSE,success=FALSE,iter_state=FALSE;
int i=0,size=0;
const gchar *wtitle;

wtitle=gtk_window_get_title(GTK_WINDOW(mwindow));
if(!g_str_has_prefix(wtitle,"*")) return;

iconview=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),"iconview");
model=gtk_icon_view_get_model(GTK_ICON_VIEW(iconview));
table=g_array_sized_new (FALSE, FALSE, sizeof (IconProperties), 1);
if(read_icon_theme(table,(const char *)current_theme)){
for(i=0;i<table->len;i++){
properties=g_array_index(table,IconProperties,i);
if(!strcmp(current_context,properties.context)){
iter_state=gtk_tree_model_get_iter_first(model,&iter);
while(iter_state){
gtk_tree_model_get (model, &iter,C_SNAME,&sname,C_NEW,&new_icon,C_USED,&icon_used,-1);
if(icon_used){
temp=g_strconcat(sname,".svg",NULL);
source_icon=g_build_path(G_DIR_SEPARATOR_S,current_theme,properties.directory,temp,NULL);
g_free(temp);
if(g_file_test(source_icon,G_FILE_TEST_EXISTS)){
if(g_str_has_suffix(new_icon,".svg")){
source=g_file_new_for_path(new_icon);
dest=g_file_new_for_path(source_icon);
success=g_file_copy(source,dest,G_FILE_COPY_OVERWRITE,NULL,NULL,NULL,&error);
if(!success){
g_error("File copy error:%s\n",error->message);
g_error_free(error);
error=NULL;
	    }
				     }
else g_print("*.svg type icon needs *.svg type icon for replacement\n");
					       }
else{
temp=g_strconcat(sname,".png",NULL);
filename=g_build_path(G_DIR_SEPARATOR_S,current_theme,properties.directory,temp,NULL);
g_free(temp);
size=atoi(properties.size);
if(g_file_test(filename,G_FILE_TEST_EXISTS)){
pixbuf=gdk_pixbuf_new_from_file_at_size(new_icon,size,size,NULL);
if(pixbuf) gdk_pixbuf_save(pixbuf,filename,"png",&error,"compression","9",NULL);
if(error){
g_print("Pixbuf save error:%s\n",error->message);
g_error_free(error);
error=NULL;
	 }
g_object_unref(pixbuf);
					    }
    }
g_free(source_icon);
	     }
iter_state=gtk_tree_model_iter_next(model,&iter);
g_free(new_icon);
g_free(sname);
		 }
				  	       }
			 }

						      }

g_array_free(table,TRUE);
reset_window_title();
acount=0;
}

void 
save_icons_one(GtkButton *button,gpointer user_data)
{
GtkWidget *radio1,*iconview,*swindow=GTK_WIDGET(user_data);
GtkWidget *statusbar,*progressbar;
GtkTreeModel *model;
GtkTreeIter iter;
GArray *table=NULL;
IconProperties properties;
gboolean radio_state=FALSE;
GdkPixbuf *pixbuf;
gchar *message,*fulldir,*sname,*new_icon,*source_icon,*filename,*temp;
gint i,e=0,size=0,l=0;
gboolean icon_used=FALSE,success=FALSE,iter_state=FALSE;
GFile *source,*dest;
GError *error=NULL;
GTimer *timer;
gdouble elapsed;

if(!savedir) savedir=g_get_current_dir();  

radio1=(GtkWidget *)g_object_get_data(G_OBJECT(button),"radio1");
radio_state=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio1));
if(radio_state) current_theme=g_strdup(backup_theme);
if(GTK_WIDGET_VISIBLE(swindow)) gtk_widget_hide(swindow);
gtk_widget_destroy(swindow);
iconview=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),"iconview");
model=gtk_icon_view_get_model(GTK_ICON_VIEW(iconview));

statusbar=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),"statusbar");
progressbar=gtk_progress_bar_new();
gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(progressbar),GTK_PROGRESS_LEFT_TO_RIGHT); 
gtk_widget_show(progressbar);
gtk_container_add(GTK_CONTAINER(statusbar),progressbar);
timer=g_timer_new();
table=g_array_sized_new (FALSE, FALSE, sizeof (IconProperties), 1);

if(read_icon_theme(table,(const char *)current_theme)){
for(i=0;i<table->len;i++){
properties=g_array_index(table,IconProperties,i);
if(!strcmp(current_context,properties.context)){
fulldir=g_build_path(G_DIR_SEPARATOR_S,savedir,properties.directory,NULL);
if(!g_file_test(fulldir,G_FILE_TEST_EXISTS))
e=g_mkdir_with_parents(fulldir,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
if(e!=0) {
message=g_strconcat("Directory Create Error:",strerror(errno),"\nSaving icons failed",NULL);
g_free(fulldir);
g_array_free(table,TRUE);
show_message(message,GTK_MESSAGE_ERROR);
g_free(message);
return;
	 }
iter_state=gtk_tree_model_get_iter_first(model,&iter);
while(iter_state){
elapsed=g_timer_elapsed(timer,0);
if(elapsed*1000>l*150){
while(gtk_events_pending()) gtk_main_iteration();
gtk_progress_bar_pulse(GTK_PROGRESS_BAR (progressbar));
l++;
                      }
gtk_tree_model_get (model, &iter,C_SNAME,&sname,C_NEW,&new_icon,C_USED,&icon_used,-1);
if(icon_used){
temp=g_strconcat(sname,".svg",NULL);
source_icon=g_build_path(G_DIR_SEPARATOR_S,current_theme,properties.directory,temp,NULL);
g_free(temp);
if(g_file_test(source_icon,G_FILE_TEST_EXISTS)){
if(g_str_has_suffix(new_icon,".svg")){
temp=g_strconcat(sname,".svg",NULL);
filename=g_build_path(G_DIR_SEPARATOR_S,savedir,properties.directory,temp,NULL);
g_free(temp);
source=g_file_new_for_path(new_icon);
dest=g_file_new_for_path(filename);
g_free(filename);
success=g_file_copy(source,dest,G_FILE_COPY_OVERWRITE,NULL,NULL,NULL,&error);
if(!success){
g_error("File copy error:%s\n",error->message);
g_error_free(error);
error=NULL;
	    }
else create_symlink_icons(fulldir,sname,".svg");
				     }
else{
temp=g_strconcat(sname,".png",NULL);
filename=g_build_path(G_DIR_SEPARATOR_S,savedir,properties.directory,temp,NULL);
g_free(temp);
size=atoi(properties.size);
pixbuf=gdk_pixbuf_new_from_file_at_size(new_icon,size,size,&error);
if(pixbuf){ 
gdk_pixbuf_save(pixbuf,filename,"png",&error,"compression","9",NULL);
if(error) {
g_print("Pixbuf save error:%s\n",error->message);
g_error_free(error);
error=NULL;
          }
else create_symlink_icons(fulldir,sname,".png");
g_object_unref(pixbuf);
          }
else{
g_print("Pixbuf load error:%s\n",error->message);
g_error_free(error);
error=NULL;
    }
g_free(filename);
    }
					      }
else{
temp=g_strconcat(sname,".png",NULL);
filename=g_build_path(G_DIR_SEPARATOR_S,savedir,properties.directory,temp,NULL);
g_free(temp);
size=atoi(properties.size);
pixbuf=gdk_pixbuf_new_from_file_at_size(new_icon,size,size,&error);
if(pixbuf){ 
gdk_pixbuf_save(pixbuf,filename,"png",&error,"compression","9",NULL);
if(error) {
g_print("Pixbuf save error:%s\n",error->message);
g_error_free(error);
error=NULL;
          }
else create_symlink_icons(fulldir,sname,".png");
g_object_unref(pixbuf);
          }
else{
g_print("Pixbuf load error:%s\n",error->message);
g_error_free(error);
error=NULL;
    }
g_free(filename);
    }


	     }
g_free(new_icon);
g_free(sname);
iter_state=gtk_tree_model_iter_next(model,&iter);
		 }
create_icon_datas(fulldir,properties.directory);
g_free(fulldir);
					       }
			 }
						      }

g_array_free(table,TRUE);
reset_window_title();
set_widget_state("revert",FALSE);
set_widget_state("toolrevert",FALSE);
acount=0;
gtk_widget_hide(progressbar);
gtk_container_remove(GTK_CONTAINER(statusbar),progressbar);
g_timer_stop(timer);
elapsed=g_timer_elapsed(timer,0);
g_timer_destroy(timer);
g_print("Saving done in %f seconds...\n",elapsed);
}


void 
set_custom_theme(GtkFileChooserButton *widget,gpointer user_data)
{
char *filename;
filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
filename=g_path_get_dirname(filename);
current_theme=g_strdup(filename);
g_free(filename);
}

void 
set_default_theme(GtkToggleButton *button,gpointer user_data)
{
if(backup_theme)
if(gtk_toggle_button_get_active(button)) current_theme=g_strdup(backup_theme);
}

void 
set_save_method(GtkToggleButton *button,gpointer user_data)
{
GtkWidget *fchooser=GTK_WIDGET(user_data);
if(gtk_toggle_button_get_active(button)) gtk_widget_set_sensitive(fchooser,FALSE);
else gtk_widget_set_sensitive(fchooser,TRUE);
}

void 
show_icon_size(GtkFileChooser *chooser,gpointer user_data)
{
GtkWidget *label=GTK_WIDGET(user_data);
char *filename,*bname,*theme_name,*ltext=NULL;
IconProperties properties;
GArray *table=NULL;
int i,result=0;

filename=gtk_file_chooser_get_preview_filename (GTK_FILE_CHOOSER(chooser));
if(filename){
bname=g_path_get_basename(filename);
if(!strcmp(bname,"index.theme")){
theme_name=g_path_get_dirname(filename);
table=g_array_sized_new (FALSE, FALSE, sizeof (IconProperties), 1);
result=read_icon_theme(table,(const char *)theme_name);
if(result){
ltext=g_strdup("Icon Sizes:");
for(i=0;i<table->len;i++){
properties=g_array_index(table,IconProperties,i);
if(!strcmp(current_context,properties.context)){
if(!strcmp(properties.type,"Fixed")) ltext=g_strconcat(ltext,properties.size,"f-",NULL);
else ltext=g_strconcat(ltext,properties.size,"s-",NULL);
				     	       }
			 }
	  }
g_free(theme_name);
g_array_free(table,TRUE);
				}

else {
g_free(bname);
g_free(filename);
return;
     }
gtk_label_set_text(GTK_LABEL(label),ltext);
g_free(ltext);
g_free(bname);
	    }
g_free(filename);
}

void 
on_quit_activate(GtkMenuItem *menuitem,gpointer user_data)
{
free_globals();
gtk_main_quit();
}

void 
on_revert_activate(GtkMenuItem *menuitem,gpointer user_data)
{
GtkTreeModel *model;
GtkTreePath *path;
GtkCellRenderer *renderer;
GtkTreeIter iter;
GtkWidget *iconview=GTK_WIDGET(user_data);
GdkPixbuf *pixbuf;
gchar *source_icon;

if(acount){
model=gtk_icon_view_get_model(GTK_ICON_VIEW(iconview));
if(gtk_icon_view_get_cursor(GTK_ICON_VIEW(iconview),&path,&renderer)){
gtk_tree_model_get_iter (GTK_TREE_MODEL (model),&iter, path);
gtk_tree_model_get (model, &iter,C_SOURCE,&source_icon,-1);
pixbuf=gdk_pixbuf_new_from_file_at_size(source_icon,48,48,NULL);
gtk_list_store_set (GTK_LIST_STORE(model), &iter,C_PIXBUF,pixbuf,C_NEW,NULL,C_USED,FALSE,-1);
g_free(source_icon);
g_object_unref(pixbuf);
gtk_tree_path_free(path);
acount--;
								     }
	  }
if(!acount){
reset_window_title();

set_widget_state("revert",FALSE);
set_widget_state("toolrevert",FALSE);
/*
set_widget_state("save",FALSE);
set_widget_state("saveas",FALSE);
set_widget_state("toolsave",FALSE);
*/
acount=0;
           }
}

void 
on_toolsave_clicked(GtkToolButton *button,gpointer user_data)
{
GtkWidget *save=GTK_WIDGET(user_data);
gtk_widget_activate(save);
}

void 
on_toolrevert_clicked(GtkToolButton *button,gpointer user_data)
{
GtkWidget *revert=GTK_WIDGET(user_data);
gtk_widget_activate(revert);
}

void 
on_about_activate(GtkMenuItem *menuitem,gpointer user_data)
{
const char *author[]={"Çetin Tanrıöver",NULL};
const char *copyright="(C) Çetin Tanrıöver";
const char *license =
    "Gither is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU Library General\n"
    "Public License as published by the Free Software\nFoundation;"
    "either version 3 of the License, or\n(at your option) any "
    "later version.\n"
    "\n"
    "Gither is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the\nimplied warranty"
    " of MERCHANTABILITY or FITNESS\nFOR A PARTICULAR PURPOSE."
    "See the GNU Library\nGeneral Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU \nGeneral "
    "Public License along with the gither;\nsee the file"
    " COPYING.If not,write to the Free\nSoftware Foundation,"
    "Inc.51 Franklin Street, Fifth Floor,\nBoston,MA 02110-1301 USA\n";
gtk_show_about_dialog (GTK_WINDOW (mwindow),
			 "name", "gither",
			 "version", "0.4",
			 "copyright", copyright,
			 "license", license,
			 "website", "http://www.gtk-apps.org",
			 "comments", "Gnome Icon Theme Helper",
			 "authors", author,
                         "title", "About gither",
			 NULL);
}

void 
add_dropped_icon(GtkWidget *widget,GdkDragContext *context,int x, int y,
                 GtkSelectionData *seldata,guint info,guint time,gpointer user_data)
{
GdkPixbuf *pixbuf;
GError *error=NULL;
GtkTreeIter iter;
char *dfile,*title;
const char *wtitle;
GtkTreeModel *model;
static char *equal="gither_drop_started!";
static int dcn=0;
GtkTreePath *path;
int wx,wy;

dfile=g_filename_from_uri((gchar*)seldata->data,NULL,&error);
dcn++;

if(error){
g_error("g_filename_from_uri():%s\n",error->message);
g_error_free(error);
error=NULL;
return;
         }
dfile[strlen(dfile)-2]='\0';
if(g_file_test(dfile,G_FILE_TEST_IS_SYMLINK)) return gtk_drag_finish (context, FALSE, FALSE, time);
if(!strcmp(equal,dfile) && dcn==1) return;
equal=g_strdup(dfile);

pixbuf=gdk_pixbuf_new_from_file_at_size(dfile,48,48,NULL);

gtk_icon_view_convert_widget_to_bin_window_coords(GTK_ICON_VIEW(widget),x,y,&wx,&wy);
path= gtk_icon_view_get_path_at_pos(GTK_ICON_VIEW(widget),wx,wy);
model=gtk_icon_view_get_model(GTK_ICON_VIEW(widget));

if(gtk_tree_model_get_iter(model,&iter,path)){

gtk_icon_view_set_drag_dest_item(GTK_ICON_VIEW(widget),path,GTK_ICON_VIEW_DROP_INTO); 
if(pixbuf){
gtk_list_store_set (GTK_LIST_STORE(model), &iter,C_PIXBUF,pixbuf,C_NEW,dfile,C_USED,TRUE,-1);
gtk_drag_finish (context, TRUE, FALSE, time);
set_widget_state("save",TRUE);
set_widget_state("saveas",TRUE);
set_widget_state("revert",TRUE);
set_widget_state("toolsave",TRUE);
set_widget_state("toolrevert",TRUE);
if(!acount){
wtitle=(const char *)gtk_window_get_title(GTK_WINDOW(mwindow));
if(!g_str_has_prefix(wtitle,"*")){
title=g_strconcat("*",wtitle,NULL);
gtk_window_set_title(GTK_WINDOW(mwindow),title);
g_free(title);
		                 }
	   }
acount++;
 	  }
					     }
else gtk_drag_finish (context, FALSE, FALSE, time);

g_object_unref(pixbuf);			     
if(dfile) g_free(dfile);
gtk_tree_path_free(path);
}

void 
initialize_method(GtkButton *button,gpointer user_data)
{
GtkWidget *statusbar,*radio=GTK_WIDGET(user_data);
statusbar=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),"statusbar");

if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) method1=TRUE;

if(method1)
gtk_statusbar_push(GTK_STATUSBAR(statusbar),1,"Method:Method One");
else
gtk_statusbar_push(GTK_STATUSBAR(statusbar),2,"Method:Method Two");  
}

void 
set_method(GtkToggleButton *button,gpointer user_data)
{
if(gtk_toggle_button_get_active(button)) method1=TRUE;
else method1=FALSE;
}

void 
create_symlink_icons(const char *fullpath,const char *basename,const char *suffix)
{
int i;
char *fname=g_new0(char,1),*lname=g_new0(char,1),*ftemp,*ltemp;

for(i=0;i<g_slist_length(flist);i++){
if(!strcmp(basename,g_slist_nth_data(flist,i))){
ftemp=g_strconcat(g_slist_nth_data(flist,i),suffix,NULL);
ltemp=g_strconcat(g_slist_nth_data(llist,i),suffix,NULL);
fname=g_build_path(G_DIR_SEPARATOR_S,fullpath,ftemp,NULL);
lname=g_build_path(G_DIR_SEPARATOR_S,fullpath,ltemp,NULL);
g_free(ftemp);
g_free(ltemp);
if(g_file_test(fname,G_FILE_TEST_EXISTS)) {
/*
if(g_file_test(lname,G_FILE_TEST_IS_SYMLINK))
if(g_remove(lname)==-1) g_print("Couldn't remove file %s\n",lname); 
*/
if(!g_file_test(lname,G_FILE_TEST_EXISTS))
if(symlink(fname,lname)) g_error("symlink error:%s\n",strerror(errno));
					  }


					  
						}
				    }
g_free(fname);
g_free(lname);
}

void 
create_icon_datas(const char *fullpath,const char *contextdir)
{
GDir *dir;
GError *error=NULL;
char *filename,*sourcedir,*readname,*contents,*checkn;
char *bname,*target;
GSList *list1=NULL,*list2=NULL;
gboolean is_symlink=FALSE;
int i;

sourcedir=g_build_path(G_DIR_SEPARATOR_S,current_theme,contextdir,NULL);
dir=g_dir_open(sourcedir,0,&error);
if(error){
g_print("Directory open error:%s\n",error->message);
g_error_free(error);
error=NULL;
g_free(sourcedir);
return;
         } 

readname=g_new(char,1);
while(readname){
readname=(char *)g_dir_read_name(dir);
if(!readname) break;
if(strstr(readname,".icon")) {
checkn=remove_suffix(readname);
if(sort_store(checkn)){
filename=g_strconcat(sourcedir,G_DIR_SEPARATOR_S,readname,NULL);
is_symlink=g_file_test(filename,G_FILE_TEST_IS_SYMLINK);
if(is_symlink){
list1=g_slist_append(list1,g_strdup(g_file_read_link(filename,NULL)));
list2=g_slist_append(list2,g_strdup(filename));
              }
else{
if(!g_file_get_contents(filename,&contents,NULL,&error)) {
g_print("Read error:%s\n",error->message);
g_error_free(error);
error=NULL;
                                                         }
else{
filename=g_strconcat(fullpath,G_DIR_SEPARATOR_S,readname,NULL);
if(!g_file_set_contents(filename,contents,-1,&error)) {
g_print("Write error:%s\n",error->message);
g_error_free(error);
error=NULL;
                                                      }
g_free(contents);
g_free(filename);
    }

    }
                      }
g_free(checkn);                      
                             } 


	       }	
g_free(sourcedir);
g_dir_close(dir);

if(list1){
for(i=0;i<g_slist_length(list1);i++){
bname=g_path_get_basename(g_slist_nth_data(list1,i));
bname=g_strconcat(fullpath,G_DIR_SEPARATOR_S,bname,NULL);
target=g_path_get_basename(g_slist_nth_data(list2,i));
target=g_strconcat(fullpath,G_DIR_SEPARATOR_S,target,NULL);
if(g_file_test(bname,G_FILE_TEST_EXISTS))
if(symlink(bname,target)) g_print("symlink() error-2:%s\n",strerror(errno));
g_free(target);
g_free(bname);
                                    }
         }
if(list1){
g_slist_foreach(list1,(GFunc)g_free,NULL);
g_slist_free(list1);
list1=NULL;
         }
if(list2){
g_slist_foreach(list2,(GFunc)g_free,NULL);
g_slist_free(list2);
list2=NULL;
         }
}

gboolean 
combo_separator(GtkTreeModel *model,GtkTreeIter *iter,gpointer data)
{
char *other=NULL;
gboolean state=FALSE;
gtk_tree_model_get (model, iter,1,&other,-1);
if(other)
if(!strcmp(other,"separator")) state=TRUE;
g_free(other);
return state;
}

char 
*remove_suffix(const char *name)
{
char *temp,*temp1;
int l=0,i;
temp=g_strdup(name);
temp1=g_strdup(name);
while(*temp1){
if(*temp1=='.') break;
temp1++;
l++;
            }
temp[l]='\0';
for(i=0;i<l;i++) temp1--;
g_free(temp1);
return temp;
}

void 
set_save_dir(GtkFileChooser *chooser,gpointer user_data)
{
savedir=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (chooser));
}

void 
show_message(const char *message,GtkMessageType message_type)
{
GtkWidget *info_bar,*label,*content_area,*statusbar;
info_bar = gtk_info_bar_new_with_buttons (GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
content_area=gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar));
gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar), message_type);
label = gtk_label_new (message);
gtk_widget_show(label);
gtk_container_add (GTK_CONTAINER (content_area), label);
statusbar=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),"statusbar");
gtk_container_add(GTK_CONTAINER(statusbar),info_bar);
g_signal_connect (info_bar, "response", G_CALLBACK (gtk_widget_hide),NULL);
gtk_widget_show(info_bar);
}

int
default_sort_func (GtkTreeModel *model,
	   GtkTreeIter  *iter1,
	   GtkTreeIter  *iter2,
	   gpointer      user_data)
{
int ret=0;  
gchar *bname1, *bname2;
  
gtk_tree_model_get (model, iter1,C_NAME,&bname1,-1);
gtk_tree_model_get (model, iter2,C_NAME,&bname2,-1);
  
bname1=g_utf8_normalize(bname1,-1,G_NORMALIZE_ALL); 
bname2=g_utf8_normalize(bname2,-1,G_NORMALIZE_ALL); 
ret=g_utf8_collate (bname1,bname2);
g_free (bname1);
g_free (bname2);
  
return  ret;
}

int 
sort_store(const char *name)
{
GtkWidget *iconview;
GtkTreeModel *model;
GtkTreeIter iter;
gboolean iter_state=FALSE,icon_used=FALSE; 
char *bname;
int yn=0;

iconview=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),"iconview");
model=gtk_icon_view_get_model(GTK_ICON_VIEW(iconview));

iter_state=gtk_tree_model_get_iter_first(model,&iter);
while(iter_state){
gtk_tree_model_get (model,&iter,C_NAME,&bname,C_USED,&icon_used,-1);
if(icon_used){
if(!strcmp(name,remove_suffix(bname))) {
yn=1;
break;
                                       }
	     }
iter_state=gtk_tree_model_iter_next(model,&iter);
	     

		 }
g_free(bname);
return yn;
}

void 
reset_window_title(void)
{
const char *wtitle;
char *mtitle;
wtitle=gtk_window_get_title(GTK_WINDOW(mwindow));
/*following code added at feb 10 2011*/
if(g_str_has_prefix(wtitle,"*"))
mtitle=g_strdup(wtitle+1);
else
mtitle=g_strdup(wtitle);
gtk_window_set_title(GTK_WINDOW(mwindow),mtitle);
g_free(mtitle);
}

void 
set_widget_state(const char *widget_name,gboolean widget_state)
{
GtkWidget *widget;
widget=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),widget_name);
gtk_widget_set_sensitive(widget,widget_state);
}

void 
free_globals(void)
{
if(theme_list){
g_slist_foreach(theme_list,(GFunc)g_free,NULL);
g_slist_free(theme_list);
theme_list=NULL;
	      } 
if(flist){
g_slist_foreach(flist,(GFunc)g_free,NULL);
g_slist_free(flist);
flist=NULL;
	 } 
if(llist){
g_slist_foreach(llist,(GFunc)g_free,NULL);
g_slist_free(llist);
llist=NULL;
	 } 
if(current_theme) g_free(current_theme);
if(backup_theme)  g_free(backup_theme);
if(current_context) g_free(current_context);
if(savedir) g_free(savedir);
}

gboolean 
delete_event(GtkWidget *widget,GdkEvent *event,gpointer data)
{
const char *wtitle;
GtkWidget *dialog,*label,*save;
int res_idi;

wtitle=gtk_window_get_title(GTK_WINDOW(mwindow));
if(g_str_has_prefix(wtitle,"*")){
save=(GtkWidget *)g_object_get_data(G_OBJECT(mwindow),"save");
dialog = gtk_dialog_new_with_buttons(NULL,GTK_WINDOW (mwindow),
GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
GTK_STOCK_SAVE,0,GTK_STOCK_QUIT,1,NULL);
label=gtk_label_new("Quit without saving?");
gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),label);
gtk_widget_set_size_request(dialog,270,130);
gtk_widget_show_all (dialog);
res_idi=gtk_dialog_run(GTK_DIALOG(dialog));
if(res_idi==0) {
gtk_widget_destroy(dialog);
gtk_widget_activate(save);
return TRUE;
               }
else{
gtk_widget_destroy(dialog);
free_globals();
return FALSE;
    }
				}
return FALSE;
}

int
display_message(const char *message,GError *error,char *data)
{
g_error("%s:%s\n",message,error->message);
g_error_free(error);
error=NULL;
if(data) g_free(data);
return 0;
}

void
_display_message(const char *message,GError *error,char *data)
{
g_error("%s:%s\n",message,error->message);
g_error_free(error);
error=NULL;
if(data) g_free(data);
}

void 
on_help_activate(GtkMenuItem *menuitem,gpointer user_data)
{
GtkWidget *window1,*textview;
GtkBuilder *builder;
GtkTextBuffer *buffer;
GtkTextIter iter;


builder=gtk_builder_new();
gtk_builder_add_from_string(builder,(const char *)gither_help_ui,-1,NULL);
gtk_builder_connect_signals(builder,NULL);

window1=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
textview=GTK_WIDGET(gtk_builder_get_object(builder,"textview1"));
g_object_unref(builder);
gtk_window_set_title(GTK_WINDOW(window1),_("Help"));
gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (textview),GTK_WRAP_WORD);     
buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
gtk_text_buffer_create_tag (buffer, "bold",
			      "weight", PANGO_WEIGHT_BOLD, NULL);
gtk_text_buffer_create_tag (buffer, "big",
			      "size", 10 * PANGO_SCALE, NULL);

gtk_text_buffer_create_tag (buffer, "blue",
			      "foreground", "blue", NULL);

gtk_text_buffer_create_tag (buffer, "red",
			      "foreground", "red", NULL);

gtk_text_buffer_create_tag (buffer, "center",
			      "justification", GTK_JUSTIFY_CENTER, NULL);

gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "GITHER HELP\n", -1,
					    "bold","big","blue","center", NULL);
gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Introduction\n", -1,
					    "bold","blue", NULL);
gtk_text_buffer_insert(buffer,&iter,"Gither is a Gnome Icon Theme Helper.Gither provides two method to use, namely method one and method two.With the help of these methods you can create new ones or change existing icon themes easily.\n",-1);
gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Method One:\n", -1,
					    "bold", NULL);
gtk_text_buffer_insert(buffer,&iter,"Method One creates new icon theme based on reference icon theme.You can thought that it is simply a copy paste process applied on all icon theme but the resulted icon set will have your new icons.\n",-1);
gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Method Two:\n", -1,
					    "bold", NULL);
gtk_text_buffer_insert(buffer,&iter,"Method Two is a simply an icon replacement.Your new icons replaced by the new ones through all sizes.This is a file overwrite process.\n",-1);

gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Usage\n", -1,
					    "bold","blue", NULL);
gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Method One:\n", -1,
					    "bold", NULL);
gtk_text_buffer_insert(buffer,&iter,"Activate File->New and choose Method One.Activate File->Open and select an icon theme and context.Drag and drop your icons.(Hint:For easy drag and drop open your new icons in a 'Folder Window' within Nautilus) When you done activate File->Save.(or click Save Icon on toolbar) Choose an empty directory if exist or create a new one within FileChooser window.Select either the default or other 'index.theme' file from the FileChooser window.The save sizes and directory structure will be determined by the 'index.theme' files.\n",-1);
gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Method Two:\n", -1,
					    "bold", NULL);
gtk_text_buffer_insert(buffer,&iter,"Activate File->New and choose Method Two.Activate File->Open and select an icon theme and context.Drag and drop your icons.(Hint:For easy drag and drop open your new icons in a 'Folder Window' within Nautilus) When you done activate File->Save.(or click Save Icon on toolbar)Your icons will be replaced.\n",-1);

gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Notes\n", -1,
					    "bold","blue", NULL);
gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Method One:\n", -1,
					    "bold", NULL);
gtk_text_buffer_insert(buffer,&iter,"If the 'index.theme' file you choosed have a scalable icon types(*.svg) use a *.svg type new icon set if available to get a same sized and type icon set.Otherwise the resulted icon set's scalable part will be saved as *.png at size provided at 'index.theme' file you choose.\n",-1);
gtk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Method Two:\n", -1,
					    "bold", NULL);
gtk_text_buffer_insert(buffer,&iter,"If the target icons are *.svg type you must exactly to provide *.svg type icon to gither.Otherwise the scalable parts (*.svg) of your icons will not changed.If the target icon set is a *.png type you can use either *.png or *.svg type icon.\n",-1);

gtk_widget_show_all(window1);
}
