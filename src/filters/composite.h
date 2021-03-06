/** @file
 * @brief SVG composite filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef SP_FECOMPOSITE_H_SEEN
#define SP_FECOMPOSITE_H_SEEN

#include "sp-filter-primitive.h"

#define SP_FECOMPOSITE(obj) (dynamic_cast<SPFeComposite*>((SPObject*)obj))
#define SP_IS_FECOMPOSITE(obj) (dynamic_cast<const SPFeComposite*>((SPObject*)obj) != NULL)

enum FeCompositeOperator {
    // Default value is 'over', but let's distinquish specifying the
    // default and implicitely using the default
    COMPOSITE_DEFAULT,
    COMPOSITE_OVER,
    COMPOSITE_IN,
    COMPOSITE_OUT,
    COMPOSITE_ATOP,
    COMPOSITE_XOR,
    COMPOSITE_ARITHMETIC,
    COMPOSITE_ENDOPERATOR
};

class SPFeComposite : public SPFilterPrimitive {
public:
	SPFeComposite();
	virtual ~SPFeComposite();

    FeCompositeOperator composite_operator;
    double k1, k2, k3, k4;
    int in2;

protected:
    virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual void build_renderer(Inkscape::Filters::Filter* filter);
};

#endif /* !SP_FECOMPOSITE_H_SEEN */

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
