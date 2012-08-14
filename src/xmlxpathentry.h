#ifndef __XML_XPATH_ENTRY_H__
#define __XML_XPATH_ENTRY_H__

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define XML_TYPE_XPATH_ENTRY    (_xml_xpathentry_get_type ())
#define XML_XPATH_ENTRY(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TYPE_XPATH_ENTRY, XmlXpathEntry))
#define XML_IS_XPATH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_TYPE_XPATH_ENTRY))

typedef struct _XmlXpathEntry      XmlXpathEntry;

GType              _xml_xpathentry_get_type           (void) G_GNUC_CONST;
GtkWidget *        _xml_xpathentry_new                (gboolean             eat_tab);
/*
void               _xml_xpathentry_set_action         (XmlXpathEntry *chooser_entry,
							       XmlXpathAction action);
XmlXpathAction _xml_xpathentry_get_action       (XmlXpathEntry *chooser_entry);
*/
void               _xml_xpathentry_set_base_folder    (XmlXpathEntry *chooser_entry,
							       GFile               *folder);
GFile *            _xml_xpathentry_get_current_folder (XmlXpathEntry *chooser_entry);
const gchar *      _xml_xpathentry_get_file_part      (XmlXpathEntry *chooser_entry);
gboolean           _xml_xpathentry_get_is_folder      (XmlXpathEntry *chooser_entry,
							       GFile               *file);
void               _xml_xpathentry_select_filename    (XmlXpathEntry *chooser_entry);
void               _xml_xpathentry_set_local_only     (XmlXpathEntry *chooser_entry,
                                                               gboolean             local_only);
gboolean           _xml_xpathentry_get_local_only     (XmlXpathEntry *chooser_entry);

G_END_DECLS

#endif /* __XML_XPATH_ENTRY_H__ */
