/*
** Generated by X-Designer
*/
#ifndef _uidepend_h
#define _uidepend_h

#define XD_MOTIF

#include <xdclass.h>

class depend_shell_c: public xd_XmDialog_c {
public:
	virtual void create (Widget parent, char *widget_name = NULL);
protected:
	Widget depend_shell;
	Widget form_;
	Widget button_close;
	Widget hyper_;
public:
	static void hyperCB( Widget, XtPointer, XtPointer );
	virtual void hyperCB( Widget, XtPointer ) = 0;
	static void closeCB( Widget, XtPointer, XtPointer );
	virtual void closeCB( Widget, XtPointer ) = 0;
};

typedef depend_shell_c *depend_shell_p;


extern depend_shell_p depend_shell;


#endif