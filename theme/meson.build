sassc = find_program('sassc', required: true)

builtin_variants = [
	'dark',
	'gruvbox',
	'light'
]

themes = []

foreach variant: builtin_variants
	themes += custom_target('@0@ theme generation'.format(variant),
		input: 'koto-builtin-@0@.scss'.format(variant),
		output: 'koto-builtin-@0@.css'.format(variant),
		command: [
			sassc,
			[ '-a', '-M', '-t', 'compact' ],
			'@INPUT@', '@OUTPUT@',
		],
		build_by_default: true,
	)
endforeach