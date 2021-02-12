koto_sources = [
	'indexer/file.c',
	'indexer/file-indexer.c',
	'main.c',
	'koto-button.c',
	'koto-expander.c',
	'koto-headerbar.c',
	'koto-nav.c',
	'koto-playerbar.c',
	'koto-window.c',
]

koto_deps = [
	dependency('glib-2.0', version: '>= 2.66'),
	dependency('gio-2.0', version: '>= 2.66'),
	dependency('gtk+-3.0', version: '>= 3.24'),
	dependency('libmagic', version: '>=5.39'),
]

gnome = import('gnome')

koto_sources += gnome.compile_resources('koto-resources',
	'koto.gresource.xml',
	c_name: 'koto'
)

executable('com.github.joshstrobl.koto', koto_sources,
	dependencies: koto_deps,
	install: true,
)
