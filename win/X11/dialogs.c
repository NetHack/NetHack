/*
 * Copyright 1991 University of Wisconsin-Madison
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Wisconsin-Madison
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The University of
 * Wisconsin-Madison makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE UNIVERSITY OF WISCONSIN-MADISON DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF WISCONSIN-MADISON BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Tim Theisen             Department of Computer Sciences
 *          tim@cs.wisc.edu         University of Wisconsin-Madison
 *          uwvax!tim               1210 West Dayton Street
 *          (608)262-0438           Madison, WI   53706
 *
 *
 * Modified 12/91 by Dean Luick.  Tim graciously donated this piece of code
 * from his program ghostview, an X11 front end for ghostscript.
 *
 *    + Make the cancel button optional.
 *    + Put an #ifdef SPECIAL_CMAP around code to fix a colormap bug.
 *      We don't need it here.
 *    + Add the function positionpopup() from another part of ghostview
 *      to this code.
 *
 * Modified 2/93, Various.
 *    + Added workaround for SYSV include problem.
 *    + Changed the default width response text widget to be as wide as the
 *      window itself.  Suggestion from David E. Wexelblat, dwex@goblin.org.
 *
 * Modified 5/2015, anonymous.
 *    + Include nethack's lint.h to get nhStr() macro.
 *    + Use nhStr() on string literals (or macros from <X11/StringDefs.h>
 *      that hide string literals) to cast away implicit 'const' in order
 *      to suppress "warning: assignment discards qualifers from pointer
 *      target type" issued by 'gcc -Wwrite-strings' as used by nethack.
 *      (For this file, always the second parameter to XtSetArg().)
 *
 * Modified 1/2016, Pat Rankin.
 *    + Added minimum width argument to SetDialogResponse() so that the
 *      text entry widget can be forced to wider than the default response.
 *    + Make 'okay' button same width as 'cancel', and both wider than
 *      default by a small arbitrary amount.
 *
 * $NHDT-Date: 1455157470 2016/02/11 02:24:30 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.9 $
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#define X11_BUILD
#include "config.h" /* #define for const for non __STDC__ compilers */
#undef X11_BUILD

#include "lint.h"   /* for nethack's nhStr() macro */
#include "winX.h"   /* to make sure protoypes match corresponding functions */

/* ":" added to both translations below to allow limited redefining of
 * keysyms before testing for keysym values -- dlc */
static const char okay_accelerators[] = "#override\n\
     :<Key>Return: set() notify() unset()\n";

static const char cancel_accelerators[] = "#override\n\
     :<Key>Escape: set() notify() unset()\n\
     :<Ctrl>[: set() notify() unset()\n"; /* for keyboards w/o an ESC */

/* Create a dialog widget.  It is just a form widget with
 *      a label prompt
 *      a text response
 *      an okay button
 *      an optional cancel button
 */
Widget
CreateDialog(Widget parent, String name, XtCallbackProc okay_callback,
             XtCallbackProc cancel_callback)
{
    Widget form, prompt, response, okay, cancel;
    Arg args[20];
    Cardinal num_args;
    Dimension owidth, cwidth;

    num_args = 0;
#ifdef SPECIAL_CMAP
    if (special_cmap) {
        XtSetArg(args[num_args], nhStr(XtNbackground), white); num_args++;
    }
#endif
    form = XtCreateManagedWidget(name, formWidgetClass, parent,
                                 args, num_args);

    num_args = 0;
#ifdef SPECIAL_CMAP
    if (special_cmap) {
        XtSetArg(args[num_args], nhStr(XtNforeground), black); num_args++;
        XtSetArg(args[num_args], nhStr(XtNbackground), white); num_args++;
    }
#endif
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNresizable), True); num_args++;
    XtSetArg(args[num_args], nhStr(XtNborderWidth), 0); num_args++;
    prompt = XtCreateManagedWidget("prompt", labelWidgetClass, form,
                                   args, num_args);

    num_args = 0;
#ifdef SPECIAL_CMAP
    if (special_cmap) {
        XtSetArg(args[num_args], nhStr(XtNforeground), black); num_args++;
        XtSetArg(args[num_args], nhStr(XtNbackground), white); num_args++;
    }
#endif
    XtSetArg(args[num_args], nhStr(XtNfromVert), prompt); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNresizable), True); num_args++;
    XtSetArg(args[num_args], nhStr(XtNeditType), XawtextEdit); num_args++;
    XtSetArg(args[num_args], nhStr(XtNresize), XawtextResizeWidth); num_args++;
    XtSetArg(args[num_args], nhStr(XtNstring), ""); num_args++;
    response = XtCreateManagedWidget("response", asciiTextWidgetClass, form,
                                     args, num_args);

    num_args = 0;
#ifdef SPECIAL_CMAP
    if (special_cmap) {
        XtSetArg(args[num_args], nhStr(XtNforeground), black); num_args++;
        XtSetArg(args[num_args], nhStr(XtNbackground), white); num_args++;
    }
#endif
    XtSetArg(args[num_args], nhStr(XtNfromVert), response); num_args++;
    XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
    XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
    XtSetArg(args[num_args], nhStr(XtNresizable), True); num_args++;
    XtSetArg(args[num_args], nhStr(XtNaccelerators),
             XtParseAcceleratorTable(okay_accelerators)); num_args++;
    okay = XtCreateManagedWidget("okay", commandWidgetClass, form,
                                 args, num_args);
    XtAddCallback(okay, XtNcallback, okay_callback, form);
    XtSetArg(args[0], XtNwidth, &owidth);
    XtGetValues(okay, args, ONE);

    /* Only create cancel button if there is a callback for it. */
    if (cancel_callback) {
        num_args = 0;
#ifdef SPECIAL_CMAP
        if (special_cmap) {
            XtSetArg(args[num_args], nhStr(XtNforeground), black); num_args++;
            XtSetArg(args[num_args], nhStr(XtNbackground), white); num_args++;
        }
#endif
        XtSetArg(args[num_args], nhStr(XtNfromVert), response); num_args++;
        XtSetArg(args[num_args], nhStr(XtNfromHoriz), okay); num_args++;
        XtSetArg(args[num_args], nhStr(XtNtop), XtChainTop); num_args++;
        XtSetArg(args[num_args], nhStr(XtNbottom), XtChainTop); num_args++;
        XtSetArg(args[num_args], nhStr(XtNleft), XtChainLeft); num_args++;
        XtSetArg(args[num_args], nhStr(XtNright), XtChainLeft); num_args++;
        XtSetArg(args[num_args], nhStr(XtNresizable), True); num_args++;
        XtSetArg(args[num_args], nhStr(XtNaccelerators),
                 XtParseAcceleratorTable(cancel_accelerators)); num_args++;
        cancel = XtCreateManagedWidget("cancel", commandWidgetClass, form,
                                       args, num_args);
        XtAddCallback(cancel, XtNcallback, cancel_callback, form);
        XtInstallAccelerators(response, cancel);
        XtSetArg(args[0], XtNwidth, &cwidth);
        XtGetValues(cancel, args, ONE);
        /* widen the cancel button */
        cwidth += 25;
        XtSetArg(args[0], XtNwidth, cwidth);
        XtSetValues(cancel, args, ONE);
    } else
        cwidth = owidth + 25;

    /* make okay button same width as cancel, or widen it if no cancel */
    XtSetArg(args[0], XtNwidth, cwidth);
    XtSetValues(okay, args, ONE);

    XtInstallAccelerators(response, okay);
    XtSetKeyboardFocus(form, response);

    return form;
}

#if 0
/* get the prompt from the dialog box.  Used a startup time to
 * save away the initial prompt */
String
GetDialogPrompt(Widget w)
{
    Arg args[1];
    Widget label;
    String s;

    label = XtNameToWidget(w, "prompt");
    XtSetArg(args[0], nhStr(XtNlabel), &s);
    XtGetValues(label, args, ONE);
    return XtNewString(s);
}
#endif

/* set the prompt.  This is used to put error information in the prompt */
void
SetDialogPrompt(Widget w, String newprompt)
{
    Arg args[1];
    Widget label;

    label = XtNameToWidget(w, "prompt");
    XtSetArg(args[0], nhStr(XtNlabel), newprompt);
    XtSetValues(label, args, ONE);
}

/* get what the user typed; caller must free the response */
String
GetDialogResponse(Widget w)
{
    Arg args[1];
    Widget response;
    String s;

    response = XtNameToWidget(w, "response");
    XtSetArg(args[0], nhStr(XtNstring), &s);
    XtGetValues(response, args, ONE);
    return XtNewString(s);
}

/* set the default reponse */
void
SetDialogResponse(Widget w, String s, unsigned ln)
{
    Arg args[4];
    Widget response;
    XFontStruct *font;
    Dimension width, nwidth, leftMargin, rightMargin;
    unsigned s_len = strlen(s);

    if (s_len < ln)
        s_len = ln;
    response = XtNameToWidget(w, "response");
    XtSetArg(args[0], nhStr(XtNfont), &font);
    XtSetArg(args[1], nhStr(XtNleftMargin), &leftMargin);
    XtSetArg(args[2], nhStr(XtNrightMargin), &rightMargin);
    XtSetArg(args[3], nhStr(XtNwidth), &width);
    XtGetValues(response, args, FOUR);
    /* width includes margins as per Xaw documentation */
    nwidth = font->max_bounds.width * (s_len + 1) + leftMargin + rightMargin;
    if (nwidth < width)
        nwidth = width;

    XtSetArg(args[0], nhStr(XtNstring), s);
    XtSetArg(args[1], nhStr(XtNwidth), nwidth);
    XtSetValues(response, args, TWO);
    XawTextSetInsertionPoint(response, strlen(s));
}

#if 0
/* clear the response */
void
ClearDialogResponse(Widget w)
{
    Arg args[2];
    Widget response;

    response = XtNameToWidget(w, "response");
    XtSetArg(args[0], nhStr(XtNstring), "");
    XtSetArg(args[1], nhStr(XtNwidth), 100);
    XtSetValues(response, args, TWO);
}
#endif

/* Not a part of the original dialogs.c from ghostview -------------------- */

/* position popup window under the cursor */
void
positionpopup(Widget w, boolean bottom) /* position y on bottom? */
{
    Arg args[3];
    Cardinal num_args;
    Dimension width, height, b_width;
    int x, y, max_x, max_y;
    Window root, child;
    XSizeHints *hints;
    int dummyx, dummyy;
    unsigned int dummymask;
    extern Widget toplevel;

    /* following line deals with a race condition w/brain-damaged WM's -dlc */
    XtUnrealizeWidget(w);

    XQueryPointer(XtDisplay(toplevel), XtWindow(toplevel), &root, &child, &x,
                  &y, &dummyx, &dummyy, &dummymask);
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, &width); num_args++;
    XtSetArg(args[num_args], XtNheight, &height); num_args++;
    XtSetArg(args[num_args], XtNborderWidth, &b_width); num_args++;
    XtGetValues(w, args, num_args);

    /* position so that the cursor is center,center or center,bottom */
    width += 2 * b_width;
    x -= ((Position) width / 2);
    if (x < 0)
        x = 0;
    if (x > (max_x = (Position) (XtScreen(w)->width - width)))
        x = max_x;

    if (bottom) {
        y -= (height + b_width - 1);
        height += 2 * b_width;
    } else {
        height += 2 * b_width;
        y -= ((Position) height / 2);
    }
    if (y < 0)
        y = 0;
    if (y > (max_y = (Position) (XtScreen(w)->height - height)))
        y = max_y;

    num_args = 0;
    XtSetArg(args[num_args], XtNx, x); num_args++;
    XtSetArg(args[num_args], XtNy, y); num_args++;
    XtSetValues(w, args, num_args);

    /* Some older window managers ignore XtN{x,y}; hint the same values.
       {x,y} are not used by newer window managers; older ones need them. */
    XtRealizeWidget(w);
    hints = XAllocSizeHints();
    hints->flags = USPosition;
    hints->x = x;
    hints->y = y;
    XSetWMNormalHints(XtDisplay(w), XtWindow(w), hints);
    XFree(hints);
}

/*dialogs.c*/
