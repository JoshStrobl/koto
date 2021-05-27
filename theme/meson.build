sassc = find_program('sassc', required: true)

theme = custom_target('Theme generation',
	input: 'main.scss',
	output: 'style.css',
	command: [
		sassc,
		[ '-a', '-M', '-t', 'compact' ],
		'@INPUT@', '@OUTPUT@',
	],
	build_by_default: true,
)
