#include <gtk/gtk.h>

GtkWidget *mwindow;

GtkTreeModel *create_model(void);

GtkTreeModel *create_model_open(void);

void on_new_activate(GtkMenuItem *menuitem,gpointer user_data);

void on_open_activate(GtkMenuItem *menuitem,gpointer user_data);

void on_save_activate(GtkMenuItem *menuitem,gpointer user_data);

void on_quit_activate(GtkMenuItem *menuitem,gpointer user_data);

void on_revert_activate(GtkMenuItem *menuitem,gpointer user_data);

void on_help_activate(GtkMenuItem *menuitem,gpointer user_data);

void on_about_activate(GtkMenuItem *menuitem,gpointer user_data);

void on_toolsave_clicked(GtkToolButton *button,gpointer user_data);

void on_toolrevert_clicked(GtkToolButton *button,gpointer user_data);

void add_dropped_icon(GtkWidget *widget,GdkDragContext *context,int x, int y,
                      GtkSelectionData *seldata,guint info,guint time,gpointer user_data);

void select_all_clicked(GtkButton *button,gpointer user_data);

void fill_model_open(GtkWidget *widget1,GtkWidget *widget2,GtkTreeModel *model);

void open_icons(char *fullpath);

void set_method(GtkToggleButton *button,gpointer user_data);

void set_save_method(GtkToggleButton *button,gpointer user_data);

void set_default_theme(GtkToggleButton *button,gpointer user_data);

void initialize_method(GtkButton *button,gpointer user_data);

void create_symlink_icons(const char *fullpath,const char *basename,const char *suffix);

void create_icon_datas(const char *fullpath,const char *contextdir);

void set_save_dir(GtkFileChooser *chooser,gpointer user_data);

void show_icon_size(GtkFileChooser *chooser,gpointer user_data);

void get_selected_theme(GtkWidget *widget,gpointer user_data);

char *get_other_theme(GtkWidget *widget);

char *check_icon_theme(const char *icon_theme,int *is_valid);

int read_icon_theme(GArray *array,const char *icon_theme);

int display_message(const char *message,GError *error,char *data);

void _display_message(const char *message,GError *error,char *data);

void show_message(const char *message,GtkMessageType message_type);

int sort_context_list(GSList *list,const char *data);

void open_icon_theme(GtkButton *button,gpointer user_data);

int default_sort_func (GtkTreeModel *model,GtkTreeIter  *iter1,GtkTreeIter  *iter2,gpointer user_data);

int sort_store(const char *name);

gboolean combo_separator(GtkTreeModel *model,GtkTreeIter *iter,gpointer data);

void save_icons_one(GtkButton *button,gpointer user_data);

void save_icons_two(void);

char *remove_suffix(const char *name);

void free_globals(void);

int set_active_theme(void);

void set_custom_theme(GtkFileChooserButton *widget,gpointer user_data);

void reset_window_title(void);

void set_widget_state(const char *widget_name,gboolean widget_state);

gboolean delete_event(GtkWidget *widget,GdkEvent *event,gpointer data);
