#define USE_TCL_STUBS
#include <tcl.h>
#define USE_TCLOO_STUBS
#include <tclOO.h>
#define PDC_WIDE
#include <sdl2/pdcsdl.h>

#include "keycodes.inc"

#if defined(_WIN32)
#define PUBLIC __declspec(dllexport) 
#define UNUSED __pragma(warning(suppress: 4100 4101))
#elif defined(__GNUC__)
#define PUBLIC __attribute__ ((visibility ("default")))
#define UNUSED __attribute__ ((unused))
#endif

#define ASSERT(cond) do if (!(cond)) printf("Invalid assertion at " __FILE__ ":%d: " #cond "\n", __LINE__); while (0)
#define ASSERT_TCLOK(expr) do if ((expr) == TCL_ERROR) printf("TCL_ERROR at " __FILE__ ":%d: " #expr ": %s\n", __LINE__, Tcl_GetStringResult(interp)); while (0)

#define TCL_METHODTYPE(CLASS, METHOD) \
static Tcl_MethodType pdc_##CLASS##_##METHOD##_type = \
{ \
	TCL_OO_METHOD_VERSION_CURRENT, \
	"PDCurses method: \"" #METHOD "\"", \
	pdc_##CLASS##_##METHOD, \
	NULL, \
	NULL, \
}

static void pure_void(UNUSED ClientData window)
{
	return;
}

static Tcl_ObjectMetadataType pdc_window_meta =
{
	TCL_OO_METADATA_VERSION_CURRENT,
	"PDcurses meta: \"window\"",
	pure_void,
	NULL,
};

enum pdc_option_t
{
	PDC_OPTION_CBREAK,
	PDC_OPTION_ECHO,
	PDC_OPTION_NL,
	PDC_OPTION_TRACE,
};

static int get_vector2_from_obj(
	Tcl_Interp* interp,
	Tcl_Obj* const obj,
	int vector[2])
{
	int len;
	Tcl_Obj** list;
	if (Tcl_ListObjGetElements(interp, obj, &len, &list) == TCL_ERROR)
		return TCL_ERROR;
	if (len != 2)
	{
		Tcl_SetResult(interp, "invalid position: should be \"{y x}\"", TCL_STATIC);
		return TCL_ERROR;
	}

	for (int i = 0; i < len; ++i)
	{
		if (Tcl_GetIntFromObj(interp, list[i], vector + i) == TCL_ERROR)
		{
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}

static int pdc_window_add(
	UNUSED ClientData clientData,
	Tcl_Interp* interp,
	Tcl_ObjectContext context,
	int objc,
	Tcl_Obj* const objv[])
{
	WINDOW* win = Tcl_ObjectGetMetadata(Tcl_ObjectContextObject(context), &pdc_window_meta);
	bool moving = false;
	int position[2];

	int skipped_argc = Tcl_ObjectContextSkippedArgs(context);
	int argc = objc - skipped_argc;
	Tcl_Obj* const* argv = objv + skipped_argc;

	if (argc == 2)
	{
		moving = true;
		if (get_vector2_from_obj(interp, *argv, position) == TCL_ERROR)
			return TCL_ERROR;
		++argv;
	}
	else if (argc != 1)
	{
		Tcl_WrongNumArgs(interp, skipped_argc, objv, "?position? string");
		return TCL_ERROR;
	}

	if (moving)
		mvwaddstr(win, position[0], position[1], Tcl_GetString(*argv));
	else
		waddstr(win, Tcl_GetString(*argv));

	return TCL_OK;
}
TCL_METHODTYPE(window, add);

static int pdc_window_getch(
	UNUSED ClientData clientData,
	Tcl_Interp* interp,
	Tcl_ObjectContext context,
	int objc,
	Tcl_Obj* const objv[])
{
	WINDOW* win = Tcl_ObjectGetMetadata(Tcl_ObjectContextObject(context), &pdc_window_meta);

	int skipped_argc = Tcl_ObjectContextSkippedArgs(context);
	if (objc - skipped_argc != 0)
	{
		Tcl_WrongNumArgs(interp, skipped_argc, objv, NULL);
		return TCL_ERROR;
	}

	wint_t intchar;
	int getret = wget_wch(win, &intchar);
	ASSERT(intchar != WEOF);
	if (getret == OK)
	{
		wchar_t character[2] = { intchar, L'\0' };
		char utf8[TCL_UTF_MAX + 1];
		PDC_wcstombs(utf8, character, TCL_UTF_MAX + 1);
		Tcl_SetResult(interp, utf8, TCL_VOLATILE);
	}
	else if (getret == KEY_CODE_YES)
	{
		ASSERT(intchar - KEY_MIN < sizeof keycodes);
		Tcl_SetObjResult(interp, Tcl_NewStringObj(keycodes[intchar - KEY_MIN], -1));
	}
	else
	{
		ASSERT(getret == ERR);
	}
	return TCL_OK;
}
TCL_METHODTYPE(window, getch);

static int pdc_window_opt(
	UNUSED ClientData clientData,
	Tcl_Interp* interp,
	Tcl_ObjectContext context,
	int objc,
	Tcl_Obj* const objv[])
{
	WINDOW* win = Tcl_ObjectGetMetadata(Tcl_ObjectContextObject(context), &pdc_window_meta);

	int skipped_argc = Tcl_ObjectContextSkippedArgs(context);
	int argc = objc - skipped_argc;
	Tcl_Obj* const* argv = objv + skipped_argc;

	if (argc != 2)
	{
		Tcl_WrongNumArgs(interp, skipped_argc, objv, "option value");
		return TCL_ERROR;
	}
	Tcl_Obj* ret;
	int value;
	const char* option_str = Tcl_GetString(argv[0]);
	if (!strcmp(option_str, "keypad"))
	{
		if (Tcl_GetBooleanFromObj(interp, argv[1], &value) == TCL_ERROR)
			return TCL_ERROR;
		ASSERT(keypad(win, value) == OK);
	}
	else if (!strcmp(option_str, "delay"))
	{
		if (Tcl_GetBooleanFromObj(interp, argv[1], &value) == TCL_ERROR)
			return TCL_ERROR;
		ASSERT(nodelay(win, !value) == OK);
	}
	else if (!strcmp(option_str, "timeout"))
	{
		ASSERT_TCLOK(Tcl_GetIntFromObj(interp, argv[1], &value));
		wtimeout(win, value);
	}
	else if (!strcmp(option_str, "immedok"))
	{
		if (Tcl_GetBooleanFromObj(interp, argv[1], &value) == TCL_ERROR)
			return TCL_ERROR;
		immedok(win, value);
	}
	else if (!strcmp(option_str, "leaveok"))
	{
		if (Tcl_GetBooleanFromObj(interp, argv[1], &value) == TCL_ERROR)
			return TCL_ERROR;
		ASSERT(leaveok(win, value) == OK);
	}
	else if (!strcmp(option_str, "scrreg"))
	{
		int pos[2];
		if (get_vector2_from_obj(interp, argv[1], pos) == TCL_ERROR)
			return TCL_ERROR;
		ASSERT(wsetscrreg(win, pos[0], pos[1]) == OK);
	}
	else if (!strcmp(option_str, "scrollok"))
	{
		if (Tcl_GetBooleanFromObj(interp, argv[1], &value) == TCL_ERROR)
			return TCL_ERROR;
		ASSERT(scrollok(win, value) == OK);
	}
	else if (!strcmp(option_str, "syncok"))
	{
		if (Tcl_GetBooleanFromObj(interp, argv[1], &value) == TCL_ERROR)
			return TCL_ERROR;
		ASSERT(syncok(win, value) == OK);
	}
	else
	{
		ret = Tcl_Format(interp, "unknown option %s: must be keypad, delay, timeout, immedok, leaveok, scrreg, scrollok, or syncok", 1, argv);
		Tcl_SetObjResult(interp, ret);
		return TCL_ERROR;
	}
	return TCL_OK;
}
TCL_METHODTYPE(window, opt);

static int pdc_opt(
	UNUSED ClientData clientData,
	Tcl_Interp* interp,
	int objc,
	Tcl_Obj* const objv[])
{
	if (objc != 3)
	{
		Tcl_WrongNumArgs(interp, 1, objv, "option value");
		return TCL_ERROR;
	}

	Tcl_Obj* ret;
	int value;
	const char* option_str = Tcl_GetString(objv[1]);
	enum pdc_option_t option;
	if (!strcmp(option_str, "cbreak"))
		option = PDC_OPTION_CBREAK;
	else if (!strcmp(option_str, "echo"))
		option = PDC_OPTION_ECHO;
	else if (!strcmp(option_str, "nl"))
		option = PDC_OPTION_NL;
	else if (!strcmp(option_str, "trace"))
		option = PDC_OPTION_TRACE;
	else
	{
		ret = Tcl_Format(interp, "unknown option %s: must be cbreak, echo, nl, or trace", 1, objv + 1);
		Tcl_SetObjResult(interp, ret);
		return TCL_ERROR;
	}
	if (Tcl_GetBooleanFromObj(interp, objv[2], &value) == TCL_ERROR)
		return TCL_ERROR;
	switch (option)
	{
		case PDC_OPTION_CBREAK:
			if (value)
				cbreak();
			else
				nocbreak();
			break;
		case PDC_OPTION_ECHO:
			if (value)
				echo();
			else
				noecho();
			break;
		case PDC_OPTION_NL:
			if (value)
				nl();
			else
				nonl();
			break;
		case PDC_OPTION_TRACE:
			if (value)
				traceon();
			else
				traceoff();
	}
	return TCL_OK;
}

PUBLIC int Tclpdc_Init(Tcl_Interp *interp)
{
	initscr();
	if (has_colors())
	{
		if (start_color() != OK)
			return TCL_ERROR;
	}
	printf("%s\n%s (%s)\n", curses_version(), termname(), longname());

	if (!Tcl_InitStubs(interp, "8.6", 0) || !Tcl_OOInitStubs(interp))
		return TCL_ERROR;

	Tcl_Class pdc_window;
	{
		Tcl_Class oo_class = Tcl_GetObjectAsClass(Tcl_GetObjectFromObj(interp, Tcl_NewStringObj("oo::class", -1)));
		Tcl_Object pdc_window_object = Tcl_NewObjectInstance(
			interp,
			oo_class,
			"pdc::window",
			NULL,
			0,
			NULL,
			0);
		pdc_window = Tcl_GetObjectAsClass(pdc_window_object);
	}

	Tcl_NewMethod(interp, pdc_window, Tcl_NewStringObj("add", -1), 1, &pdc_window_add_type, NULL);
	Tcl_NewMethod(interp, pdc_window, Tcl_NewStringObj("getch", -1), 1, &pdc_window_getch_type, NULL);
	Tcl_NewMethod(interp, pdc_window, Tcl_NewStringObj("opt", -1), 1, &pdc_window_opt_type, NULL);
	Tcl_CreateObjCommand(interp, "pdc::opt", pdc_opt, NULL, NULL);

	Tcl_Object pdc_stdscr = Tcl_NewObjectInstance(interp, pdc_window, "pdc::stdscr", NULL, 0, NULL, 0);
	Tcl_ObjectSetMetadata(pdc_stdscr, &pdc_window_meta, stdscr);

	Tcl_Object pdc_curscr = Tcl_NewObjectInstance(interp, pdc_window, "pdc::curscr", NULL, 0, NULL, 0);
	Tcl_ObjectSetMetadata(pdc_curscr, &pdc_window_meta, curscr);

	return Tcl_PkgProvide(interp, "PDCurses", "1.0");
}
