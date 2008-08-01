/*
 * A simple utility for exporting Inkscape svg Shapes as PovRay bezier
 * prisms.  Note that this is output-only, and would thus seem to be
 * better placed as an 'export' rather than 'output'.  However, Export
 * handles all or partial documents, while this outputs ALL shapes in
 * the current SVG document.
 *
 *  For information on the PovRay file format, see:
 *      http://www.povray.org
 *
 * Authors:
 *   Bob Jamison <ishmal@inkscape.org>
 *
 * Copyright (C) 2004-2008 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "pov-out.h"
#include <inkscape.h>
#include <inkscape_version.h>
#include <sp-path.h>
#include <style.h>
#include <display/curve.h>
#include <libnr/n-art-bpath.h>
#include <extension/system.h>
#include <2geom/pathvector.h>
#include <2geom/rect.h>
#include <2geom/bezier-curve.h>
#include <2geom/hvlinesegment.h>
#include "helper/geom.h"
#include <io/sys.h>

#include <string>
#include <stdio.h>
#include <stdarg.h>


namespace Inkscape
{
namespace Extension
{
namespace Internal
{




//########################################################################
//# U T I L I T Y
//########################################################################



/**
 * This function searches the Repr tree recursively from the given node,
 * and adds refs to all nodes with the given name, to the result vector
 */
static void
findElementsByTagName(std::vector<Inkscape::XML::Node *> &results,
                      Inkscape::XML::Node *node,
                      char const *name)
{
    if ( !name || strcmp(node->name(), name) == 0 )
        results.push_back(node);

    for (Inkscape::XML::Node *child = node->firstChild() ; child ;
              child = child->next())
        findElementsByTagName( results, child, name );

}





static double
effective_opacity(SPItem const *item)
{
    double ret = 1.0;
    for (SPObject const *obj = item; obj; obj = obj->parent)
        {
        SPStyle const *const style = SP_OBJECT_STYLE(obj);
        g_return_val_if_fail(style, ret);
        ret *= SP_SCALE24_TO_FLOAT(style->opacity.value);
        }
    return ret;
}





//########################################################################
//# OUTPUT FORMATTING
//########################################################################


/**
 * We want to control floating output format
 */
static PovOutput::String dstr(double d)
{
    char dbuf[G_ASCII_DTOSTR_BUF_SIZE+1];
    g_ascii_formatd(dbuf, G_ASCII_DTOSTR_BUF_SIZE,
                  "%.8f", (gdouble)d);
    PovOutput::String s = dbuf;
    return s;
}




/**
 *  Output data to the buffer, printf()-style
 */
void PovOutput::out(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gchar *output = g_strdup_vprintf(fmt, args);
    va_end(args);
    outbuf.append(output);
    g_free(output);
}





/**
 *  Output a 2d vector
 */
void PovOutput::vec2(double a, double b)
{
    out("<%s, %s>", dstr(a).c_str(), dstr(b).c_str());
}



/**
 * Output a 3d vector
 */
void PovOutput::vec3(double a, double b, double c)
{
    out("<%s, %s, %s>", dstr(a).c_str(), dstr(b).c_str(), dstr(c).c_str());
}



/**
 *  Output a v4d ector
 */
void PovOutput::vec4(double a, double b, double c, double d)
{
    out("<%s, %s, %s, %s>", dstr(a).c_str(), dstr(b).c_str(),
	         dstr(c).c_str(), dstr(d).c_str());
}



/**
 *  Output an rgbf color vector
 */
void PovOutput::rgbf(double r, double g, double b, double f)
{
    //"rgbf < %1.3f, %1.3f, %1.3f %1.3f>"
    out("rgbf ");
    vec4(r, g, b, f);
}



/**
 *  Output one bezier's start, start-control, end-control, and end nodes
 */
void PovOutput::segment(int segNr,
                        double startX,     double startY,
                        double startCtrlX, double startCtrlY,
                        double endCtrlX,   double endCtrlY,
                        double endX,       double endY)
{
    //"    /*%4d*/ <%f, %f>, <%f, %f>, <%f,%f>, <%f,%f>"
    out("    /*%4d*/ ", segNr);
    vec2(startX,     startY);
    out(", ");
    vec2(startCtrlX, startCtrlY);
    out(", ");
    vec2(endCtrlX,   endCtrlY);
    out(", ");
    vec2(endX,       endY);
}





/**
 * Output the file header
 */
bool PovOutput::doHeader()
{
    time_t tim = time(NULL);
    out("/*###################################################################\n");
    out("### This PovRay document was generated by Inkscape\n");
    out("### http://www.inkscape.org\n");
    out("### Created: %s", ctime(&tim));
    out("### Version: %s\n", INKSCAPE_VERSION);
    out("#####################################################################\n");
    out("### NOTES:\n");
    out("### ============\n");
    out("### POVRay information can be found at\n");
    out("### http://www.povray.org\n");
    out("###\n");
    out("### The 'AllShapes' objects at the bottom are provided as a\n");
    out("### preview of how the output would look in a trace.  However,\n");
    out("### the main intent of this file is to provide the individual\n");
    out("### shapes for inclusion in a POV project.\n");
    out("###\n");
    out("### For an example of how to use this file, look at\n");
    out("### share/examples/istest.pov\n");
    out("###\n");
    out("### If you have any problems with this output, please see the\n");
    out("### Inkscape project at http://www.inkscape.org, or visit\n");
    out("### the #inkscape channel on irc.freenode.net . \n");
    out("###\n");
    out("###################################################################*/\n");
    out("\n\n");
    out("/*###################################################################\n");
    out("##   Exports in this file\n");
    out("##==========================\n");
    out("##    Shapes   : %d\n", nrShapes);
    out("##    Segments : %d\n", nrSegments);
    out("##    Nodes    : %d\n", nrNodes);
    out("###################################################################*/\n");
    out("\n\n\n");
    return true;
}



/**
 *  Output the file footer
 */
bool PovOutput::doTail()
{
    out("\n\n");
    out("/*###################################################################\n");
    out("### E N D    F I L E\n");
    out("###################################################################*/\n");
    out("\n\n");
    return true;
}



/**
 *  Output the curve data to buffer
 */
bool PovOutput::doCurve(SPItem *item, const String &id)
{
    using Geom::X;
    using Geom::Y;

    //### Get the Shape
    if (!SP_IS_SHAPE(item))//Bulia's suggestion.  Allow all shapes
        return true;

    SPShape *shape = SP_SHAPE(item);
    SPCurve *curve = shape->curve;
    if (curve->is_empty())
        return true;

    nrShapes++;

    PovShapeInfo shapeInfo;
    shapeInfo.id    = id;
    shapeInfo.color = "";

    //Try to get the fill color of the shape
    SPStyle *style = SP_OBJECT_STYLE(shape);
    /* fixme: Handle other fill types, even if this means translating gradients to a single
           flat colour. */
    if (style)
        {
        if (style->fill.isColor())
            {
            // see color.h for how to parse SPColor
            float rgb[3];
            sp_color_get_rgb_floatv(&style->fill.value.color, rgb);
            double const dopacity = ( SP_SCALE24_TO_FLOAT(style->fill_opacity.value)
                                      * effective_opacity(shape) );
            //gchar *str = g_strdup_printf("rgbf < %1.3f, %1.3f, %1.3f %1.3f>",
            //                             rgb[0], rgb[1], rgb[2], 1.0 - dopacity);
            String rgbf = "rgbf <";
            rgbf.append(dstr(rgb[0]));         rgbf.append(", ");
            rgbf.append(dstr(rgb[1]));         rgbf.append(", ");
            rgbf.append(dstr(rgb[2]));         rgbf.append(", ");
            rgbf.append(dstr(1.0 - dopacity)); rgbf.append(">");
            shapeInfo.color += rgbf;
            }
        }

    povShapes.push_back(shapeInfo); //passed all tests.  save the info

    // convert the path to only lineto's and cubic curveto's:
    Geom::Matrix tf = sp_item_i2d_affine(item);
    Geom::PathVector pathv = pathv_to_linear_and_cubic_beziers( curve->get_pathvector() * tf );

    //Count the NR_CURVETOs/LINETOs (including closing line segment)
    int segmentCount = 0;
    for(Geom::PathVector::const_iterator it = pathv.begin(); it != pathv.end(); ++it)
	    {
        segmentCount += (*it).size();
        if (it->closed())
            segmentCount += 1;
        }

    out("/*###################################################\n");
    out("### PRISM:  %s\n", id.c_str());
    out("###################################################*/\n");
    out("#declare %s = prism {\n", id.c_str());
    out("    linear_sweep\n");
    out("    bezier_spline\n");
    out("    1.0, //top\n");
    out("    0.0, //bottom\n");
    out("    %d //nr points\n", segmentCount * 4);
    int segmentNr = 0;

    nrSegments += segmentCount;

    /**
     *	 at moment of writing, 2geom lacks proper initialization of empty intervals in rect...
     */     
    Geom::Rect cminmax( pathv.front().initialPoint(), pathv.front().initialPoint() ); 
   
   
    /**
     * For all Subpaths in the <path>
     */	     
    for (Geom::PathVector::const_iterator pit = pathv.begin(); pit != pathv.end(); ++pit)
        {

        cminmax.expandTo(pit->initialPoint());

        /**
         * For all segments in the subpath
         */		         
        for (Geom::Path::const_iterator cit = pit->begin(); cit != pit->end_closed(); ++cit)
		    {

            if( dynamic_cast<Geom::LineSegment const *> (&*cit) ||
                    dynamic_cast<Geom::HLineSegment const *>(&*cit) ||
                    dynamic_cast<Geom::VLineSegment const *>(&*cit) )
                {
                Geom::Point p0 = cit->initialPoint();
                Geom::Point p1 = cit->finalPoint();
                segment(segmentNr++,
                        p0[X], p0[Y], p0[X], p0[Y], p1[X], p1[Y], p1[X], p1[Y] );
                nrNodes += 8;
                }
            else if(Geom::CubicBezier const *cubic = dynamic_cast<Geom::CubicBezier const*>(&*cit))
			    {
                std::vector<Geom::Point> points = cubic->points();
                Geom::Point p0 = points[0];
                Geom::Point p1 = points[1];
                Geom::Point p2 = points[2];
                Geom::Point p3 = points[3];
                segment(segmentNr++,
                            p0[X],p0[Y], p1[X],p1[Y], p2[X],p2[Y], p3[X],p3[Y]);
                nrNodes += 8;
                }
            else
			    {
                g_warning("logical error, because pathv_to_linear_and_cubic_beziers was used");
                return false;
                }

            if (segmentNr < segmentCount)
                out(",\n");
            else
                out("\n");

            cminmax.expandTo(cit->finalPoint());

            }
        }

    out("}\n");

    double cminx = cminmax.min()[X];
    double cmaxx = cminmax.max()[X];
    double cminy = cminmax.min()[Y];
    double cmaxy = cminmax.max()[Y];

    out("#declare %s_MIN_X    = %s;\n", id.c_str(), dstr(cminx).c_str());
    out("#declare %s_CENTER_X = %s;\n", id.c_str(), dstr((cmaxx+cminx)/2.0).c_str());
    out("#declare %s_MAX_X    = %s;\n", id.c_str(), dstr(cmaxx).c_str());
    out("#declare %s_WIDTH    = %s;\n", id.c_str(), dstr(cmaxx-cminx).c_str());
    out("#declare %s_MIN_Y    = %s;\n", id.c_str(), dstr(cminy).c_str());
    out("#declare %s_CENTER_Y = %s;\n", id.c_str(), dstr((cmaxy+cminy)/2.0).c_str());
    out("#declare %s_MAX_Y    = %s;\n", id.c_str(), dstr(cmaxy).c_str());
    out("#declare %s_HEIGHT   = %s;\n", id.c_str(), dstr(cmaxy-cminy).c_str());
    if (shapeInfo.color.length()>0)
        out("#declare %s_COLOR    = %s;\n",
                id.c_str(), shapeInfo.color.c_str());
    out("/*###################################################\n");
    out("### end %s\n", id.c_str());
    out("###################################################*/\n\n\n\n");

    if (cminx < minx)
        minx = cminx;
    if (cmaxx > maxx)
        maxx = cmaxx;
    if (cminy < miny)
        miny = cminy;
    if (cmaxy > maxy)
        maxy = cmaxy;

    return true;
}

/**
 *  Output the curve data to buffer
 */
bool PovOutput::doCurvesRecursive(SPDocument *doc, Inkscape::XML::Node *node)
{
    /**
     * If the object is an Item, try processing it
     */	     
    char *str  = (char *) node->attribute("id");
    SPObject *reprobj = doc->getObjectByRepr(node);
    if (SP_IS_ITEM(reprobj) && str)
        {
        SPItem *item = SP_ITEM(reprobj);
        String id = str;
        if (!doCurve(item, id))
            return false;
        }

    /**
     * Descend into children
     */	     
    for (Inkscape::XML::Node *child = node->firstChild() ; child ;
              child = child->next())
        {
		if (!doCurvesRecursive(doc, child))
		    return false;
		}

    return true;
}

/**
 *  Output the curve data to buffer
 */
bool PovOutput::doCurves(SPDocument *doc)
{
    double bignum = 1000000.0;
    minx  =  bignum;
    maxx  = -bignum;
    miny  =  bignum;
    maxy  = -bignum;

    if (!doCurvesRecursive(doc, doc->rroot))
        return false;

    //## Let's make a union of all of the Shapes
    if (povShapes.size()>0)
        {
        String id = "AllShapes";
        char *pfx = (char *)id.c_str();
        out("/*###################################################\n");
        out("### UNION OF ALL SHAPES IN DOCUMENT\n");
        out("###################################################*/\n");
        out("\n\n");
        out("/**\n");
        out(" * Allow the user to redefine the finish{}\n");
        out(" * by declaring it before #including this file\n");
        out(" */\n");
        out("#ifndef (%s_Finish)\n", pfx);
        out("#declare %s_Finish = finish {\n", pfx);
        out("    phong 0.5\n");
        out("    reflection 0.3\n");
        out("    specular 0.5\n");
        out("}\n");
        out("#end\n");
        out("\n\n");
        out("#declare %s = union {\n", id.c_str());
        for (unsigned i = 0 ; i < povShapes.size() ; i++)
            {
            out("    object { %s\n", povShapes[i].id.c_str());
            out("        texture { \n");
            if (povShapes[i].color.length()>0)
                out("            pigment { %s }\n", povShapes[i].color.c_str());
            else
                out("            pigment { rgb <0,0,0> }\n");
            out("            finish { %s_Finish }\n", pfx);
            out("            } \n");
            out("        } \n");
            }
        out("}\n\n\n\n");


        double zinc   = 0.2 / (double)povShapes.size();
        out("/*#### Same union, but with Z-diffs (actually Y in pov) ####*/\n");
        out("\n\n");
        out("/**\n");
        out(" * Allow the user to redefine the Z-Increment\n");
        out(" */\n");
        out("#ifndef (AllShapes_Z_Increment)\n");
        out("#declare AllShapes_Z_Increment = %s;\n", dstr(zinc).c_str());
        out("#end\n");
        out("\n");
        out("#declare AllShapes_Z_Scale = 1.0;\n");
        out("\n\n");
        out("#declare %s_Z = union {\n", pfx);

        for (unsigned i = 0 ; i < povShapes.size() ; i++)
            {
            out("    object { %s\n", povShapes[i].id.c_str());
            out("        texture { \n");
            if (povShapes[i].color.length()>0)
                out("            pigment { %s }\n", povShapes[i].color.c_str());
            else
                out("            pigment { rgb <0,0,0> }\n");
            out("            finish { %s_Finish }\n", pfx);
            out("            } \n");
            out("        scale <1, %s_Z_Scale, 1>\n", pfx);
            out("        } \n");
            out("#declare %s_Z_Scale = %s_Z_Scale + %s_Z_Increment;\n\n",
                    pfx, pfx, pfx);
            }

        out("}\n");

        out("#declare %s_MIN_X    = %s;\n", pfx, dstr(minx).c_str());
        out("#declare %s_CENTER_X = %s;\n", pfx, dstr((maxx+minx)/2.0).c_str());
        out("#declare %s_MAX_X    = %s;\n", pfx, dstr(maxx).c_str());
        out("#declare %s_WIDTH    = %s;\n", pfx, dstr(maxx-minx).c_str());
        out("#declare %s_MIN_Y    = %s;\n", pfx, dstr(miny).c_str());
        out("#declare %s_CENTER_Y = %s;\n", pfx, dstr((maxy+miny)/2.0).c_str());
        out("#declare %s_MAX_Y    = %s;\n", pfx, dstr(maxy).c_str());
        out("#declare %s_HEIGHT   = %s;\n", pfx, dstr(maxy-miny).c_str());
        out("/*##############################################\n");
        out("### end %s\n", id.c_str());
        out("##############################################*/\n");
        out("\n\n");
        }

    return true;
}


//########################################################################
//# M A I N    O U T P U T
//########################################################################



/**
 *  Set values back to initial state
 */
void PovOutput::reset()
{
    nrNodes    = 0;
    nrSegments = 0;
    nrShapes   = 0;
    outbuf.clear();
    povShapes.clear();
}



/**
 * Saves the Shapes of an Inkscape SVG file as PovRay spline definitions
 */
void PovOutput::saveDocument(SPDocument *doc, gchar const *uri)
{
    reset();

    //###### SAVE IN POV FORMAT TO BUFFER
    //# Lets do the curves first, to get the stats
    if (!doCurves(doc))
        {
        g_warning("Could not output curves for %s\n", uri);
        return;
        }
        
    String curveBuf = outbuf;
    outbuf.clear();

    if (!doHeader())
        {
        g_warning("Could not write header for %s\n", uri);
        return;
        }

    outbuf.append(curveBuf);

    if (!doTail())
        {
        g_warning("Could not write footer for %s\n", uri);
        return;
        }




    //###### WRITE TO FILE
    Inkscape::IO::dump_fopen_call(uri, "L");
    FILE *f = Inkscape::IO::fopen_utf8name(uri, "w");
    if (!f)
        return;

    for (String::iterator iter = outbuf.begin() ; iter!=outbuf.end(); iter++)
        {
        int ch = *iter;
        fputc(ch, f);
        }

    fclose(f);
}




//########################################################################
//# EXTENSION API
//########################################################################



#include "clear-n_.h"



/**
 * API call to save document
*/
void
PovOutput::save(Inkscape::Extension::Output *mod,
                        SPDocument *doc, gchar const *uri)
{
    saveDocument(doc, uri);
}



/**
 * Make sure that we are in the database
 */
bool PovOutput::check (Inkscape::Extension::Extension *module)
{
    /* We don't need a Key
    if (NULL == Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_POV))
        return FALSE;
    */

    return true;
}



/**
 * This is the definition of PovRay output.  This function just
 * calls the extension system with the memory allocated XML that
 * describes the data.
*/
void
PovOutput::init()
{
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("PovRay Output") "</name>\n"
            "<id>org.inkscape.output.pov</id>\n"
            "<output>\n"
                "<extension>.pov</extension>\n"
                "<mimetype>text/x-povray-script</mimetype>\n"
                "<filetypename>" N_("PovRay (*.pov) (export splines)") "</filetypename>\n"
                "<filetypetooltip>" N_("PovRay Raytracer File") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>",
        new PovOutput());
}





}  // namespace Internal
}  // namespace Extension
}  // namespace Inkscape


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
