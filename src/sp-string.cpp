/*
 * SVG <text> and <tspan> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * fixme:
 *
 * These subcomponents should not be items, or alternately
 * we have to invent set of flags to mark, whether standard
 * attributes are applicable to given item (I even like this
 * idea somewhat - Lauris)
 *
 */



#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include "sp-string.h"
#include "xml/repr.h"


/*#####################################################
#  SPSTRING
#####################################################*/

static void sp_string_class_init(SPStringClass *classname);
static void sp_string_init(SPString *string);

static void sp_string_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_string_release(SPObject *object);
static void sp_string_read_content(SPObject *object);
static void sp_string_update(SPObject *object, SPCtx *ctx, unsigned flags);

static SPObjectClass *string_parent_class;

GType
sp_string_get_type()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPStringClass),
            NULL,    /* base_init */
            NULL,    /* base_finalize */
            (GClassInitFunc) sp_string_class_init,
            NULL,    /* class_finalize */
            NULL,    /* class_data */
            sizeof(SPString),
            16,    /* n_preallocs */
            (GInstanceInitFunc) sp_string_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static(SP_TYPE_OBJECT, "SPString", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_string_class_init(SPStringClass *classname)
{
    SPObjectClass *sp_object_class;

    sp_object_class = (SPObjectClass *) classname;

    string_parent_class = (SPObjectClass*)g_type_class_ref(SP_TYPE_OBJECT);

    sp_object_class->build        = sp_string_build;
    sp_object_class->release      = sp_string_release;
    sp_object_class->read_content = sp_string_read_content;
    sp_object_class->update       = sp_string_update;
}

CString::CString(SPString* str) : CObject(str) {
	this->spstring = str;
}

CString::~CString() {
}

static void
sp_string_init(SPString *string)
{
	string->cstring = new CString(string);
	string->cobject = string->cstring;

    new (&string->string) Glib::ustring();
}

void CString::onBuild(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPString* object = this->spstring;
    sp_string_read_content(object);

    CObject::onBuild(doc, repr);
}

static void
sp_string_build(SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr)
{
	((SPString*)object)->cstring->onBuild(doc, repr);
}

void CString::onRelease() {
	SPString* object = this->spstring;
    SPString *string = SP_STRING(object);

    string->string.~ustring();

    CObject::onRelease();
}

static void
sp_string_release(SPObject *object)
{
	((SPString*)object)->cstring->onRelease();
}

void CString::onReadContent() {
	SPString* object = this->spstring;

    SPString *string = SP_STRING(object);

    string->string.clear();

    //XML Tree being used directly here while it shouldn't be.
    gchar const *xml_string = string->getRepr()->content();
    // see algorithms described in svg 1.1 section 10.15
    if (object->xml_space.value == SP_XML_SPACE_PRESERVE) {
        for ( ; *xml_string ; xml_string = g_utf8_next_char(xml_string) ) {
            gunichar c = g_utf8_get_char(xml_string);
            if ((c == 0xa) || (c == 0xd) || (c == '\t')) {
                c = ' ';
            }
            string->string += c;
        }
    }
    else {
        bool whitespace = false;
        for ( ; *xml_string ; xml_string = g_utf8_next_char(xml_string) ) {
            gunichar c = g_utf8_get_char(xml_string);
            if ((c == 0xa) || (c == 0xd)) {
                continue;
            }
            if ((c == ' ') || (c == '\t')) {
                whitespace = true;
            } else {
                if (whitespace && (!string->string.empty() || (object->getPrev() != NULL))) {
                    string->string += ' ';
                }
                string->string += c;
                whitespace = false;
            }
        }
        if (whitespace && object->getRepr()->next() != NULL) {   // can't use SPObject::getNext() when the SPObject tree is still being built
            string->string += ' ';
        }
    }
    object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_string_read_content(SPObject *object)
{
	((SPString*)object)->cstring->onReadContent();
}

void CString::onUpdate(SPCtx *ctx, unsigned flags) {
	// CPPIFY: This doesn't make no sense.
	// CObject::onUpdate is pure. What was the idea behind these lines?
//	if (((SPObjectClass *) string_parent_class)->update)
//	        ((SPObjectClass *) string_parent_class)->update(object, ctx, flags);
//    CObject::onUpdate(ctx, flags);

    if (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_MODIFIED_FLAG)) {
        /* Parent style or we ourselves changed, so recalculate */
        flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B; // won't be "just a transformation" anymore, we're going to recompute "x" and "y" attributes
    }
}

static void
sp_string_update(SPObject *object, SPCtx *ctx, unsigned flags)
{
	((SPString*)object)->cstring->onUpdate(ctx, flags);
}


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
