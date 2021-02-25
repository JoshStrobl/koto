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
		'pages/_music-local.scss',
		'_button.scss',
		'_vars.scss',
		'_expander.scss',
		'_player-bar.scss',
		'_primary-nav.scss',
		'_track-item.scss',
	]),
	build_by_default: true,
)
