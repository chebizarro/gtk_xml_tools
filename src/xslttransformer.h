/*
 *	Custom Widget Template
 *
 */

#ifndef __XSLT_TRANSFORMER_H__
#define __XSLT_TRANSFORMER_H__

#include <gtk/gtk.h>
#include "xmltreemodel.h"

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>


G_BEGIN_DECLS

#define XSLT_TRANSFORMER_TYPE				(xslt_transformer_get_type ())
#define XSLT_TRANSFORMER(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XSLT_TRANSFORMER_TYPE, xsltTransformer))
#define XSLT_TRANSFORMER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XSLT_TRANSFORMER_TYPE, xsltTransformerClass))
#define IS_XSLT_TRANSFORMER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XSLT_TRANSFORMER_TYPE))
#define IS_XSLT_TRANSFORMER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XSLT_TRANSFORMER_TYPE))

typedef struct _xsltTransformer			 xsltTransformer;
typedef struct _xsltTransformerClass	xsltTransformerClass;

struct _xsltTransformer
{
	GtkVBox		widget;
	
	GtkWidget	*file_entry;
	GtkWidget	*param_list_box;

};

struct _xsltTransformerClass
{
	GtkVBoxClass parent_class;

	void (* xslt_transformer) (xsltTransformer *ttt);
};

GType			xslt_transformer_get_type				(void);
GtkWidget*		xslt_transformer_new					(void);

G_END_DECLS

#endif /* __XSLT_TRANSFORMER_H__ */
