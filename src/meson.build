add_project_arguments('-Db_sanitize=address', language: 'c')

koto_sources = [
	'db/cartographer.c',
	'db/db.c',
	'indexer/album.c',
	'indexer/artist.c',
	'indexer/file-indexer.c',
	'indexer/track.c',
	'pages/music/album-view.c',
	'pages/music/artist-view.c',
	'pages/music/disc-view.c',
	'pages/music/music-local.c',
	'playback/engine.c',
	'playback/media-keys.c',
	'playback/mimes.c',
	'playback/mpris.c',
	'playlist/current.c',
	'playlist/playlist.c',
	'main.c',
	'koto-button.c',
	'koto-expander.c',
	'koto-nav.c',
	'koto-playerbar.c',
	'koto-track-item.c',
	'koto-utils.c',
	'koto-window.c',
]

koto_deps = [
	dependency('glib-2.0', version: '>= 2.66'),
	dependency('gio-2.0', version: '>= 2.66'),
	dependency('gstreamer-1.0', version: '>= 1.18'),
	dependency('gstreamer-player-1.0', version: '>= 1.18'),
	dependency('gtk4', version: '>= 4.0'),
	dependency('libmagic', version: '>=5.39'),
	dependency('sqlite3', version: '>=3.34'),
	dependency('taglib_c', version: '>=1.11'),
]

gnome = import('gnome')

koto_sources += gnome.compile_resources('koto-resources',
	'koto.gresource.xml',
	dependencies: [ theme ],
	c_name: 'koto',
)

executable('com.github.joshstrobl.koto', koto_sources,
	dependencies: koto_deps,
	install: true,
)
