## Process this file with automake to produce Makefile.in

INCLUDES = \
	@GTK_CFLAGS@

bin_PROGRAMS = glowcontrol

glowcontrol_SOURCES = \
	main.c main.h \
	main_window.c main_window.h \
	project.c project.h \
	scene.c scene.h \
	scene_editor.c scene_editor.h \
	step.c step.h \
	step_editor.c step_editor.h \
	out.c out.h \
	audio.c audio.h \
	xml_parser.c xml_parser.h \
	xml_writer.c xml_writer.h \
	remote.c remote.h \
	gfx.c gfx.h \
        types.h intl.h
        

glowcontrol_LDADD = @GTK_LIBS@ -lusb

