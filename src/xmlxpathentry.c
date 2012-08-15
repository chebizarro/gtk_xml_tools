#include "xmlmodel.h"
#include "xmlxpathentry.h"

#include <string.h>


typedef struct _XmlXpathEntryClass XmlXpathEntryClass;

#define XML_XPATH_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), XML_TYPE_XPATH_ENTRY, XmlXpathEntryClass))
#define XML_IS_XPATH_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), XML_TYPE_XPATH_ENTRY))
#define XML_XPATH_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), XML_TYPE_XPATH_ENTRY, XmlXpathEntryClass))

struct _XmlXpathEntryClass
{
  GtkEntryClass parent_class;
};

struct _XmlXpathEntry
{
  GtkEntry parent_instance;

 // XmlXpathAction action;

  GFile *base_folder;
  GFile *current_folder_file;
  gchar *dir_part;
  gchar *file_part;

  GtkTreeModel *completion_store;

  guint current_folder_loaded : 1;
  guint complete_on_load : 1;
  guint eat_tabs       : 1;
  guint local_only     : 1;
};

enum
{
  DISPLAY_NAME_COLUMN,
  FULL_PATH_COLUMN,
  N_COLUMNS
};

static void     xml_xpathentry_finalize       (GObject          *object);
static void     xml_xpathentry_dispose        (GObject          *object);
static void     xml_xpathentry_grab_focus     (GtkWidget        *widget);
static gboolean xml_xpathentry_tab_handler    (GtkWidget *widget,
						       GdkEventKey *event);
static gboolean xml_xpathentry_focus_out_event (GtkWidget       *widget,
							GdkEventFocus   *event);

#ifdef G_OS_WIN32
static gint     insert_text_callback      (XmlXpathEntry *widget,
					   const gchar         *new_text,
					   gint                 new_text_length,
					   gint                *position,
					   gpointer             user_data);
static void     delete_text_callback      (XmlXpathEntry *widget,
					   gint                 start_pos,
					   gint                 end_pos,
					   gpointer             user_data);
#endif

static gboolean match_selected_callback   (GtkEntryCompletion  *completion,
					   GtkTreeModel        *model,
					   GtkTreeIter         *iter,
					   XmlXpathEntry *chooser_entry);

static void set_complete_on_load (XmlXpathEntry *chooser_entry,
                                  gboolean             complete_on_load);
static void refresh_current_folder_and_file_part (XmlXpathEntry *chooser_entry);
static void set_completion_folder (XmlXpathEntry *chooser_entry,
                                   GFile               *folder);
static void finished_loading_cb (XmlList  *model,
                                 GError              *error,
		                 XmlXpathEntry *chooser_entry);

G_DEFINE_TYPE (XmlXpathEntry, _xml_xpathentry, GTK_TYPE_ENTRY)

static char *
xml_xpathentry_get_completion_text (XmlXpathEntry *chooser_entry)
{
  GtkEditable *editable = GTK_EDITABLE (chooser_entry);
  int start, end;

  gtk_editable_get_selection_bounds (editable, &start, &end);
  return gtk_editable_get_chars (editable, 0, MIN (start, end));
}

static void
xml_xpathentry_dispatch_properties_changed (GObject     *object,
                                                    guint        n_pspecs,
                                                    GParamSpec **pspecs)
{
  XmlXpathEntry *chooser_entry = XML_XPATH_ENTRY (object);
  guint i;

  G_OBJECT_CLASS (_xml_xpathentry_parent_class)->dispatch_properties_changed (object, n_pspecs, pspecs);

  /* What we are after: The text in front of the cursor was modified.
   * Unfortunately, there's no other way to catch this. */

  for (i = 0; i < n_pspecs; i++)
    {
      if (pspecs[i]->name == g_intern_static_string("cursor-position") ||
          pspecs[i]->name == g_intern_static_string("selection-bound") ||
          pspecs[i]->name == g_intern_static_string("text"))
        {
          set_complete_on_load (chooser_entry, FALSE);
          refresh_current_folder_and_file_part (chooser_entry);
          break;
        }
    }
}

static void
_xml_xpathentry_class_init (XmlXpathEntryClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->finalize = xml_xpathentry_finalize;
  gobject_class->dispose = xml_xpathentry_dispose;
  gobject_class->dispatch_properties_changed = xml_xpathentry_dispatch_properties_changed;

  widget_class->grab_focus = xml_xpathentry_grab_focus;
  widget_class->focus_out_event = xml_xpathentry_focus_out_event;
}

static void
_xml_xpathentry_init (XmlXpathEntry *chooser_entry)
{
  GtkEntryCompletion *comp;
  GtkCellRenderer *cell;

  chooser_entry->local_only = TRUE;

  g_object_set (chooser_entry, "truncate-multiline", TRUE, NULL);

  comp = gtk_entry_completion_new ();
  gtk_entry_completion_set_popup_single_match (comp, FALSE);
  gtk_entry_completion_set_minimum_key_length (comp, 0);
  /* see docs for gtk_entry_completion_set_text_column() */
  g_object_set (comp, "text-column", FULL_PATH_COLUMN, NULL);

  /* Need a match func here or entry completion uses a wrong one.
   * We do our own filtering after all. */
  gtk_entry_completion_set_match_func (comp,
				       (GtkEntryCompletionMatchFunc) gtk_true,
				       chooser_entry,
				       NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (comp),
                              cell, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (comp),
                                 cell,
                                 "text", DISPLAY_NAME_COLUMN);

  g_signal_connect (comp, "match-selected",
		    G_CALLBACK (match_selected_callback), chooser_entry);

  gtk_entry_set_completion (GTK_ENTRY (chooser_entry), comp);
  g_object_unref (comp);
  /* NB: This needs to happen after the completion is set, so this handler
   * runs before the handler installed by entrycompletion */
  g_signal_connect (chooser_entry, "key-press-event",
                    G_CALLBACK (xml_xpathentry_tab_handler), NULL);

#ifdef G_OS_WIN32
  g_signal_connect (chooser_entry, "insert-text",
		    G_CALLBACK (insert_text_callback), NULL);
  g_signal_connect (chooser_entry, "delete-text",
		    G_CALLBACK (delete_text_callback), NULL);
#endif
}

static void
xml_xpathentry_finalize (GObject *object)
{
  XmlXpathEntry *chooser_entry = XML_XPATH_ENTRY (object);

  if (chooser_entry->base_folder)
    g_object_unref (chooser_entry->base_folder);

  if (chooser_entry->current_folder_file)
    g_object_unref (chooser_entry->current_folder_file);

  g_free (chooser_entry->dir_part);
  g_free (chooser_entry->file_part);

  G_OBJECT_CLASS (_xml_xpathentry_parent_class)->finalize (object);
}

static void
xml_xpathentry_dispose (GObject *object)
{
  XmlXpathEntry *chooser_entry = XML_XPATH_ENTRY (object);

  set_completion_folder (chooser_entry, NULL);

  G_OBJECT_CLASS (_xml_xpathentry_parent_class)->dispose (object);
}

/* Match functions for the GtkEntryCompletion */
static gboolean
match_selected_callback (GtkEntryCompletion  *completion,
                         GtkTreeModel        *model,
                         GtkTreeIter         *iter,
                         XmlXpathEntry *chooser_entry)
{
  char *path;
  gint pos;

  gtk_tree_model_get (model, iter,
                      FULL_PATH_COLUMN, &path,
                      -1);

  gtk_editable_delete_text (GTK_EDITABLE (chooser_entry),
                            0,
                            gtk_editable_get_position (GTK_EDITABLE (chooser_entry)));
  pos = 0;
  gtk_editable_insert_text (GTK_EDITABLE (chooser_entry),
                            path,
                            -1,
                            &pos);

  g_free (path);

  return TRUE;
}

static void
set_complete_on_load (XmlXpathEntry *chooser_entry,
                      gboolean             complete_on_load)
{
  /* a completion was triggered, but we couldn't do it.
   * So no text was inserted when pressing tab, so we beep */
  if (chooser_entry->complete_on_load && !complete_on_load)
    gtk_widget_error_bell (GTK_WIDGET (chooser_entry));

  chooser_entry->complete_on_load = complete_on_load;
}

static gboolean
is_valid_scheme_character (char c)
{
  return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_uri_scheme (const char *str)
{
  const char *p;

  p = str;

  if (!is_valid_scheme_character (*p))
    return FALSE;

  do
    p++;
  while (is_valid_scheme_character (*p));

  return (strncmp (p, "://", 3) == 0);
}

static GFile *
xml_xpathget_file_for_text (XmlXpathEntry *chooser_entry,
                                    const gchar         *str)
{
  GFile *file;

  if (str[0] == '~' || g_path_is_absolute (str) || has_uri_scheme (str))
    file = g_file_parse_name (str);
  else if (chooser_entry->base_folder != NULL)
    file = g_file_resolve_relative_path (chooser_entry->base_folder, str);
  else
    file = NULL;

  return file;
}

static GFile *
xml_xpathget_directory_for_text (XmlXpathEntry *chooser_entry,
                                         const char *         text)
{
  GFile *file, *parent;

  file = xml_xpathget_file_for_text (chooser_entry, text);

  if (file == NULL)
    return NULL;

  if (text[0] == 0 || text[strlen (text) - 1] == G_DIR_SEPARATOR)
    return file;

  parent = g_file_get_parent (file);
  g_object_unref (file);

  return parent;
}

/* Finds a common prefix based on the contents of the entry
 * and mandatorily appends it
 */
static void
explicitly_complete (XmlXpathEntry *chooser_entry)
{
  chooser_entry->complete_on_load = FALSE;

  if (chooser_entry->completion_store)
    {
      char *completion, *text;
      gsize completion_len, text_len;
      
      text = xml_xpathentry_get_completion_text (chooser_entry);
      text_len = strlen (text);

      completion = gtk_entry_completion_compute_prefix (
							gtk_entry_get_completion (
								GTK_ENTRY (chooser_entry)
							),
							text);
							
      completion_len = completion ? strlen (completion) : 0;

      if (completion_len > text_len)
        {
          GtkEditable *editable = GTK_EDITABLE (chooser_entry);
          int pos = gtk_editable_get_position (editable);

          gtk_editable_insert_text (editable,
                                    completion + text_len,
                                    completion_len - text_len,
                                    &pos);
          gtk_editable_set_position (editable, pos);
          return;
        }
    }

  gtk_widget_error_bell (GTK_WIDGET (chooser_entry));
}

static void
xml_xpathentry_grab_focus (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (_xml_xpathentry_parent_class)->grab_focus (widget);
  _xml_xpathentry_select_filename (XML_XPATH_ENTRY (widget));
}

static void
start_explicit_completion (XmlXpathEntry *chooser_entry)
{
  if (chooser_entry->current_folder_loaded)
    explicitly_complete (chooser_entry);
  else
    set_complete_on_load (chooser_entry, TRUE);
}

static gboolean
xml_xpathentry_tab_handler (GtkWidget *widget,
				    GdkEventKey *event)
{
  XmlXpathEntry *chooser_entry;
  GtkEditable *editable;
  GdkModifierType state;
  gint start, end;

  chooser_entry = XML_XPATH_ENTRY (widget);
  editable = GTK_EDITABLE (widget);

  if (!chooser_entry->eat_tabs)
    return FALSE;

 if (event->keyval != GDK_KEY_Tab)
    return FALSE;

  if (gtk_get_current_event_state (&state) &&
      (state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
    return FALSE;

  /* This is a bit evil -- it makes Tab never leave the entry. It basically
   * makes it 'safe' for people to hit. */
  gtk_editable_get_selection_bounds (editable, &start, &end);
      
  if (start != end)
    gtk_editable_set_position (editable, MAX (start, end));
  else
    start_explicit_completion (chooser_entry);

  return TRUE;
}

static gboolean
xml_xpathentry_focus_out_event (GtkWidget     *widget,
					GdkEventFocus *event)
{
  XmlXpathEntry *chooser_entry = XML_XPATH_ENTRY (widget);

  set_complete_on_load (chooser_entry, FALSE);
 
  return GTK_WIDGET_CLASS (_xml_xpathentry_parent_class)->focus_out_event (widget, event);
}

static void
update_inline_completion (XmlXpathEntry *chooser_entry)
{
  GtkEntryCompletion *completion = gtk_entry_get_completion (GTK_ENTRY (chooser_entry));

  if (!chooser_entry->current_folder_loaded)
    {
      gtk_entry_completion_set_inline_completion (completion, FALSE);
      return;
    }
/*
  switch (chooser_entry->action)
    {
	
    case XML_XPATH_ACTION_OPEN:
    case XML_XPATH_ACTION_SELECT_FOLDER:
      gtk_entry_completion_set_inline_completion (completion, TRUE);
      break;
    case XML_XPATH_ACTION_SAVE:
    case XML_XPATH_ACTION_CREATE_FOLDER:
      gtk_entry_completion_set_inline_completion (completion, FALSE);

	  break;
	 
    }*/
}

static void
discard_completion_store (XmlXpathEntry *chooser_entry)
{
  if (!chooser_entry->completion_store)
    return;

  gtk_entry_completion_set_model (gtk_entry_get_completion (GTK_ENTRY (chooser_entry)), NULL);
  update_inline_completion (chooser_entry);
  g_object_unref (chooser_entry->completion_store);
  chooser_entry->completion_store = NULL;
}

static gboolean
completion_store_set (XmlList  *model,
                      GFile               *file,
                      GFileInfo           *info,
                      int                  column,
                      GValue              *value,
                      gpointer             data)
{
  XmlXpathEntry *chooser_entry = data;

  const char *prefix = "";
  const char *suffix = "";

  switch (column)
    {
    case FULL_PATH_COLUMN:
      prefix = chooser_entry->dir_part;
      /* fall through */
    case DISPLAY_NAME_COLUMN:
      if (_gtk_file_info_consider_as_directory (info))
        suffix = G_DIR_SEPARATOR_S;

      g_value_take_string (value, g_strconcat (
              prefix,
              g_file_info_get_display_name (info),
              suffix,
              NULL));
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

/* Fills the completion store from the contents of the current folder */
static void
populate_completion_store (XmlXpathEntry *chooser_entry)
{
  chooser_entry->completion_store = GTK_TREE_MODEL (
      _gtk_file_system_model_new_for_directory (chooser_entry->current_folder_file,
                                                "standard::name,standard::display-name,standard::type",
                                                completion_store_set,
                                                chooser_entry,
                                                N_COLUMNS,
                                                G_TYPE_STRING,
                                                G_TYPE_STRING));
  g_signal_connect (chooser_entry->completion_store, "finished-loading",
		    G_CALLBACK (finished_loading_cb), chooser_entry);
/*
  _gtk_file_system_model_set_filter_folders (XML_XPATH_SYSTEM_MODEL (chooser_entry->completion_store),
                                             TRUE);
  _gtk_file_system_model_set_show_files (GTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                         chooser_entry->action == XML_XPATH_ACTION_OPEN ||
                                         chooser_entry->action == XML_XPATH_ACTION_SAVE);

*/
	
gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (chooser_entry->completion_store),
					DISPLAY_NAME_COLUMN, GTK_SORT_ASCENDING);

  gtk_entry_completion_set_model (gtk_entry_get_completion (GTK_ENTRY (chooser_entry)),
				  chooser_entry->completion_store);
}

/* Callback when the current folder finishes loading */
static void
finished_loading_cb (XmlList  *model,
                     GError              *error,
		     XmlXpathEntry *chooser_entry)
{
  GtkEntryCompletion *completion;

  chooser_entry->current_folder_loaded = TRUE;

  if (error)
    {
      discard_completion_store (chooser_entry);
      set_complete_on_load (chooser_entry, FALSE);
      return;
    }

  if (chooser_entry->complete_on_load)
    explicitly_complete (chooser_entry);

  gtk_widget_set_tooltip_text (GTK_WIDGET (chooser_entry), NULL);

  completion = gtk_entry_get_completion (GTK_ENTRY (chooser_entry));
  update_inline_completion (chooser_entry);
  gtk_entry_completion_complete (completion);
  gtk_entry_completion_insert_prefix (completion);
}

static void
set_completion_folder (XmlXpathEntry *chooser_entry,
                       GFile               *folder_file)
{
  if (folder_file &&
      chooser_entry->local_only
      && !g_file_is_native (folder_file))
    folder_file = NULL;

  if ((chooser_entry->current_folder_file
       && folder_file
       && g_file_equal (folder_file, chooser_entry->current_folder_file))
      || chooser_entry->current_folder_file == folder_file)
    return;

  if (chooser_entry->current_folder_file)
    {
      g_object_unref (chooser_entry->current_folder_file);
      chooser_entry->current_folder_file = NULL;
    }
  
  chooser_entry->current_folder_loaded = FALSE;

  discard_completion_store (chooser_entry);

  if (folder_file)
    {
      chooser_entry->current_folder_file = g_object_ref (folder_file);
      populate_completion_store (chooser_entry);
    }
}

static void
refresh_current_folder_and_file_part (XmlXpathEntry *chooser_entry)
{
  GFile *folder_file;
  char *text, *last_slash, *old_file_part;

  old_file_part = chooser_entry->file_part;
  g_free (chooser_entry->dir_part);

  text = xml_xpathentry_get_completion_text (chooser_entry);

  last_slash = strrchr (text, G_DIR_SEPARATOR);
  if (last_slash)
    {
      chooser_entry->dir_part = g_strndup (text, last_slash - text + 1);
      chooser_entry->file_part = g_strdup (last_slash + 1);
    }
  else
    {
      chooser_entry->dir_part = g_strdup ("");
      chooser_entry->file_part = g_strdup (text);
    }

  folder_file = xml_xpathget_directory_for_text (chooser_entry, text);
  set_completion_folder (chooser_entry, folder_file);
  if (folder_file)
    g_object_unref (folder_file);

  if (chooser_entry->completion_store &&
      (g_strcmp0 (old_file_part, chooser_entry->file_part) != 0))
    {
      GtkFileFilter *filter;
      char *pattern;

      filter = gtk_file_filter_new ();
      pattern = g_strconcat (chooser_entry->file_part, "*", NULL);
      gtk_file_filter_add_pattern (filter, pattern);

      _gtk_file_system_model_set_filter (GTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                         filter);

      g_free (pattern);
      g_object_unref (filter);
    }

  g_free (text);
  g_free (old_file_part);
}

#ifdef G_OS_WIN32
static gint
insert_text_callback (XmlXpathEntry *chooser_entry,
		      const gchar	  *new_text,
		      gint       	   new_text_length,
		      gint       	  *position,
		      gpointer   	   user_data)
{
  const gchar *colon = memchr (new_text, ':', new_text_length);
  gint i;

  /* Disallow these characters altogether */
  for (i = 0; i < new_text_length; i++)
    {
      if (new_text[i] == '<' ||
	  new_text[i] == '>' ||
	  new_text[i] == '"' ||
	  new_text[i] == '|' ||
	  new_text[i] == '*' ||
	  new_text[i] == '?')
	break;
    }

  if (i < new_text_length ||
      /* Disallow entering text that would cause a colon to be anywhere except
       * after a drive letter.
       */
      (colon != NULL &&
       *position + (colon - new_text) != 1) ||
      (new_text_length > 0 &&
       *position <= 1 &&
       gtk_entry_get_text_length (GTK_ENTRY (chooser_entry)) >= 2 &&
       gtk_entry_get_text (GTK_ENTRY (chooser_entry))[1] == ':'))
    {
      gtk_widget_error_bell (GTK_WIDGET (chooser_entry));
      g_signal_stop_emission_by_name (chooser_entry, "insert_text");
      return FALSE;
    }

  return TRUE;
}

static void
delete_text_callback (XmlXpathEntry *chooser_entry,
		      gint                 start_pos,
		      gint                 end_pos,
		      gpointer             user_data)
{
  /* If deleting a drive letter, delete the colon, too */
  if (start_pos == 0 && end_pos == 1 &&
      gtk_entry_get_text_length (GTK_ENTRY (chooser_entry)) >= 2 &&
      gtk_entry_get_text (GTK_ENTRY (chooser_entry))[1] == ':')
    {
      g_signal_handlers_block_by_func (chooser_entry,
				       G_CALLBACK (delete_text_callback),
				       user_data);
      gtk_editable_delete_text (GTK_EDITABLE (chooser_entry), 0, 1);
      g_signal_handlers_unblock_by_func (chooser_entry,
					 G_CALLBACK (delete_text_callback),
					 user_data);
    }
}
#endif

/**
 * _xml_xpathentry_new:
 * @eat_tabs: If %FALSE, allow focus navigation with the tab key.
 *
 * Creates a new #XmlXpathEntry object. #XmlXpathEntry
 * is an internal implementation widget for the GTK+ file chooser
 * which is an entry with completion with respect to a
 * #GtkFileSystem object.
 *
 * Return value: the newly created #XmlXpathEntry
 **/
GtkWidget *
_xml_xpathentry_new (gboolean       eat_tabs)
{
  XmlXpathEntry *chooser_entry;

  chooser_entry = g_object_new (XML_TYPE_XPATH_ENTRY, NULL);
  chooser_entry->eat_tabs = (eat_tabs != FALSE);

  return GTK_WIDGET (chooser_entry);
}

/**
 * _xml_xpathentry_set_base_folder:
 * @chooser_entry: a #XmlXpathEntry
 * @file: file for a folder in the chooser entries current file system.
 *
 * Sets the folder with respect to which completions occur.
 **/
void
_xml_xpathentry_set_base_folder (XmlXpathEntry *chooser_entry,
					 GFile               *file)
{
  g_return_if_fail (XML_IS_XPATH_ENTRY (chooser_entry));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (chooser_entry->base_folder == file ||
      (file != NULL && chooser_entry->base_folder != NULL 
       && g_file_equal (chooser_entry->base_folder, file)))
    return;

  if (file)
    g_object_ref (file);

  if (chooser_entry->base_folder)
    g_object_unref (chooser_entry->base_folder);

  chooser_entry->base_folder = file;

  refresh_current_folder_and_file_part (chooser_entry);
}

/**
 * _xml_xpathentry_get_current_folder:
 * @chooser_entry: a #XmlXpathEntry
 *
 * Gets the current folder for the #XmlXpathEntry. If the
 * user has only entered a filename, this will be in the base folder
 * (see _xml_xpathentry_set_base_folder()), but if the
 * user has entered a relative or absolute path, then it will
 * be different.  If the user has entered unparsable text, or text which
 * the entry cannot handle, this will return %NULL.
 *
 * Return value: the file for the current folder - you must g_object_unref()
 *   the value after use.
 **/
GFile *
_xml_xpathentry_get_current_folder (XmlXpathEntry *chooser_entry)
{
  g_return_val_if_fail (XML_IS_XPATH_ENTRY (chooser_entry), NULL);

  return xml_xpathget_directory_for_text (chooser_entry,
                                                  gtk_entry_get_text (GTK_ENTRY (chooser_entry)));
}

/**
 * _xml_xpathentry_get_file_part:
 * @chooser_entry: a #XmlXpathEntry
 *
 * Gets the non-folder portion of whatever the user has entered
 * into the file selector. What is returned is a UTF-8 string,
 * and if a filename path is needed, g_file_get_child_for_display_name()
 * must be used
  *
 * Return value: the entered filename - this value is owned by the
 *  chooser entry and must not be modified or freed.
 **/
const gchar *
_xml_xpathentry_get_file_part (XmlXpathEntry *chooser_entry)
{
  const char *last_slash, *text;

  g_return_val_if_fail (XML_IS_XPATH_ENTRY (chooser_entry), NULL);

  text = gtk_entry_get_text (GTK_ENTRY (chooser_entry));
  last_slash = strrchr (text, G_DIR_SEPARATOR);
  if (last_slash)
    return last_slash + 1;
  else
    return text;
}

/**
 * _xml_xpathentry_set_action:
 * @chooser_entry: a #XmlXpathEntry
 * @action: the action which is performed by the file selector using this entry
 *
 * Sets action which is performed by the file selector using this entry. 
 * The #XmlXpathEntry will use different completion strategies for 
 * different actions.
 **/
/*void
_xml_xpathentry_set_action (XmlXpathEntry *chooser_entry,
				    XmlXpathAction action)
{
  g_return_if_fail (XML_IS_XPATH_ENTRY (chooser_entry));
  
  if (chooser_entry->action != action)
    {
      GtkEntryCompletion *comp;

      chooser_entry->action = action;

      comp = gtk_entry_get_completion (GTK_ENTRY (chooser_entry));


      switch (action)
	{
	case XML_XPATH_ACTION_OPEN:
	case XML_XPATH_ACTION_SELECT_FOLDER:
	  gtk_entry_completion_set_popup_single_match (comp, FALSE);
	  break;
	case XML_XPATH_ACTION_SAVE:
	case XML_XPATH_ACTION_CREATE_FOLDER:
	  gtk_entry_completion_set_popup_single_match (comp, TRUE);
	  break;
	}

      if (chooser_entry->completion_store)
        _gtk_file_system_model_set_show_files (GTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                               action == XML_XPATH_ACTION_OPEN ||
                                               action == XML_XPATH_ACTION_SAVE);

      update_inline_completion (chooser_entry);
    }
}
*/

/**
 * _xml_xpathentry_get_action:
 * @chooser_entry: a #XmlXpathEntry
 *
 * Gets the action for this entry. 
 *
 * Returns: the action
 **/
/*XmlXpathAction
_xml_xpathentry_get_action (XmlXpathEntry *chooser_entry)
{
  g_return_val_if_fail (XML_IS_XPATH_ENTRY (chooser_entry),
			XML_XPATH_ACTION_OPEN);
  
  return chooser_entry->action;
}*/

gboolean
_xml_xpathentry_get_is_folder (XmlXpathEntry *chooser_entry,
				       GFile               *file)
{
  GtkTreeIter iter;
  GFileInfo *info;

  if (chooser_entry->completion_store == NULL ||
      !_gtk_file_system_model_get_iter_for_file (GTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                                 &iter,
                                                 file))
    return FALSE;

  info = _gtk_file_system_model_get_info (GTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                          &iter);

  return _gtk_file_info_consider_as_directory (info);
}


/*
 * _xml_xpathentry_select_filename:
 * @chooser_entry: a #XmlXpathEntry
 *
 * Selects the filename (without the extension) for user edition.
 */
void
_xml_xpathentry_select_filename (XmlXpathEntry *chooser_entry)
{
  const gchar *str, *ext;
  glong len = -1;

  /*if (chooser_entry->action == XML_XPATH_ACTION_SAVE)
    {
      str = gtk_entry_get_text (GTK_ENTRY (chooser_entry));
      ext = g_strrstr (str, ".");

      if (ext)
       len = g_utf8_pointer_to_offset (str, ext);
    }*/

  gtk_editable_select_region (GTK_EDITABLE (chooser_entry), 0, (gint) len);
}

void
_xml_xpathentry_set_local_only (XmlXpathEntry *chooser_entry,
                                        gboolean             local_only)
{
  chooser_entry->local_only = local_only;
  refresh_current_folder_and_file_part (chooser_entry);
}

gboolean
_xml_xpathentry_get_local_only (XmlXpathEntry *chooser_entry)
{
  return chooser_entry->local_only;
}