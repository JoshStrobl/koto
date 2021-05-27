project('koto', 'c',
	version: '0.1.0',
	meson_version: '>= 0.57.0',
	default_options: [
		'c_std=gnu11',
		'warning_level=2',
		'werror=true',
	],
)

i18n = import('i18n')
gnome = import('gnome')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'koto')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(
	output: 'koto-config.h',
	configuration: config_h,
)

c = meson.get_compiler('c')
toml_dep = c.find_library('toml', required: true)

subdir('theme')
subdir('data')
subdir('src')
subdir('po')

gnome.post_install(
	glib_compile_schemas: true,
	gtk_update_icon_cache: false
)
meson.add_install_script('build-aux/meson/postinstall.py')
