/* eslint-disable @typescript-eslint/no-unsafe-assignment */
/* eslint-disable @typescript-eslint/no-unsafe-argument */

import preprocess from 'svelte-preprocess'
import adapter from '@sveltejs/adapter-auto'
import autoprefixer from 'autoprefixer'

const styles = ['colors', 'font', 'scroll']

/** @type {import('@sveltejs/kit').Config} */
const config = {
	preprocess: preprocess({
		postcss: {
			plugins: [autoprefixer()]
		},
		scss: {
			prependData: styles.map(path => `@use 'styles/${path}';`).join('')
		}
	}),
	compilerOptions: {
		immutable: true
	},
	kit: {
		adapter: adapter(),
		files: {
			assets: 'public',
			hooks: 'hooks',
			lib: 'lib',
			routes: 'routes',
			serviceWorker: 'lib/worker',
			template: 'lib/index.html'
		},
		csp: {
			directives: {
				'base-uri': ['self'],
				'default-src': ['self'],
				'connect-src': [
					'self',
					'https://pagead2.googlesyndication.com',
					'https://firebase.googleapis.com',
					'https://firebaseinstallations.googleapis.com',
					'https://www.google-analytics.com',
					'https://identitytoolkit.googleapis.com',
					'https://securetoken.googleapis.com',
					'https://firestore.googleapis.com'
				],
				'script-src': [
					'self',
					'https://pagead2.googlesyndication.com',
					'https://www.googletagmanager.com',
					'https://apis.google.com'
				],
				'style-src': ['self', 'unsafe-inline', 'https://fonts.googleapis.com'],
				'img-src': [
					'self',
					'https://pagead2.googlesyndication.com',
					'https://www.google.com'
				],
				'font-src': ['self', 'https://fonts.gstatic.com'],
				'frame-src': [
					'self',
					'https://googleads.g.doubleclick.net',
					'https://tpc.googlesyndication.com',
					'https://www.google.com',
					'https://gravitygolfgame.firebaseapp.com'
				]
			}
		}
	}
}

export default config
