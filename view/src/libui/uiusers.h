/*
** Generated by X-Designer
*/
#ifndef _uiusers_h
#define _uiusers_h

#define XD_MOTIF

#include <xdclass.h>

class users_form_c: public xd_XmForm_c {
public:
	virtual void create (Widget parent, char *widget_name = NULL);
protected:
	Widget users_form;
	Widget list_;
public:
	static void sendCB( Widget, XtPointer, XtPointer );
	virtual void sendCB( Widget, XtPointer ) = 0;
};

typedef users_form_c *users_form_p;


extern users_form_p users_form;


#endif