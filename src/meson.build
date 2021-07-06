add_global_arguments([
	'-I' + meson.current_build_dir(),
	'-Db_sanitize=address',
	'-Dwerror=true',
], language: 'c')

koto_sources = [
	'components/koto-action-bar.c',
	'components/koto-cover-art-button.c',
	'components/track-table.c',
	'config/config.c',
	'db/cartographer.c',
	'db/db.c',
	'db/loaders.c',
	'indexer/album.c',
	'indexer/artist.c',
	'indexer/file-indexer.c',
	'indexer/library.c',
	'indexer/track-helpers.c',
	'indexer/track.c',
	'pages/music/album-view.c',
	'pages/music/artist-view.c',
	'pages/music/disc-view.c',
	'pages/music/music-local.c',
	'pages/playlist/list.c',
	'playback/engine.c',
	'playback/media-keys.c',
	'playback/mimes.c',
	'playback/mpris.c',
	'playlist/add-remove-track-popover.c',
	'playlist/create-modify-dialog.c',
	'playlist/current.c',
	'playlist/playlist.c',
	'main.c',
	'koto-button.c',
	'koto-dialog-container.c',
	'koto-expander.c',
	'koto-nav.c',
	'koto-playerbar.c',
	'koto-paths.c',
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
	toml_dep,
]

gnome = import('gnome')

koto_sources += gnome.compile_resources('koto-resources',
	'koto.gresource.xml',
	dependencies: themes,
	c_name: 'koto',
)

executable('com.github.joshstrobl.koto', koto_sources,
	dependencies: koto_deps,
	install: true,
)
