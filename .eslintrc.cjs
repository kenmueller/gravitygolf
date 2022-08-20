module.exports = {
	root: true,
	parser: '@typescript-eslint/parser',
	extends: [
		'eslint:recommended',
		'plugin:@typescript-eslint/eslint-recommended',
		'plugin:@typescript-eslint/recommended',
		'plugin:@typescript-eslint/recommended-requiring-type-checking',
		'prettier'
	],
	plugins: ['svelte3', '@typescript-eslint'],
	overrides: [{ files: ['*.svelte'], processor: 'svelte3/svelte3' }],
	settings: {
		'svelte3/typescript': () => require('typescript'),
		'svelte3/ignore-styles': () => true
	},
	rules: {
		strict: ['error', 'never'],
		semi: ['error', 'never'],
		quotes: ['error', 'single', { avoidEscape: true }],
		'jsx-quotes': ['error', 'prefer-double'],
		'no-unused-vars': 'off',
		'@typescript-eslint/no-unused-vars': 'error',
		'@typescript-eslint/no-unsafe-call': 'off',
		'@typescript-eslint/ban-ts-comment': 'off'
	},
	globals: {
		$$restProps: 'readonly',
		MOBILE: 'readonly'
	},
	parserOptions: {
		sourceType: 'module',
		ecmaVersion: 2020,
		project: ['tsconfig.json', 'tsconfig.dev.json']
	},
	env: {
		browser: true,
		es2020: true,
		node: true
	}
}
