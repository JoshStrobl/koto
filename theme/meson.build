sassc = find_program('sassc', required: true)

theme = custom_target('Theme generation',
	input: 'main.scss',
	output: 'style.css',
	command: [
		sassc,
		[ '-a', '-M', '-t', 'compact' ],
		'@INPUT@', '@OUTPUT@',
	],
	depend_files: files([
		'components/_cover-art-button.scss',
		'components/_gtk-overrides.scss',
		'pages/_music-local.scss',
		'pages/_playlist-page.scss',
		'_button.scss',
		'_disc-view.scss',
		'_expander.scss',
		'_player-bar.scss',
		'_primary-nav.scss',
		'_track-item.scss',
		'_vars.scss',
	]),
	build_by_default: true,
)
