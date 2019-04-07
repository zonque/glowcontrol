project('glowcontrol', 'c')
gtkdep = dependency('gtk+-2.0')
usbdep = dependency('libusb')
sources = files('src/audio.c',
                'src/gfx.c',
                'src/main.c',
                'src/main_window.c',
                'src/out.c',
                'src/project.c',
                'src/remote.c',
                'src/scene.c',
                'src/scene_editor.c',
                'src/step.c',
                'src/step_editor.c',
                'src/xml_parser.c',
                'src/xml_writer.c')

pixmaps = files('pixmaps/audio.xpm',
                'pixmaps/channel.xpm',
                'pixmaps/dimm.xpm',
                'pixmaps/first.xpm',
                'pixmaps/next.xpm',
                'pixmaps/play.xpm',
                'pixmaps/plug.xpm',
                'pixmaps/prev.xpm',
                'pixmaps/scene.xpm',
                'pixmaps/sleep.xpm',
                'pixmaps/sound.xpm',
                'pixmaps/sync.xpm')

install_data(pixmaps, install_dir: 'share/glowcontrol/pixmaps')
install_data('gnome/GlowControl.png', install_dir: 'share/icons/hicolor/128x128/apps/')
install_data('gnome/GlowControl.desktop', install_dir: 'share/applications/')

add_global_arguments('-DXPMDIR="' + get_option('prefix') + '/share/glowcontrol/pixmaps/"', language : 'c')

executable('glowcontrol', sources, dependencies: [gtkdep, usbdep],
           install: true, install_dir: 'bin')