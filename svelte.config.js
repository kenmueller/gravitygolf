/* eslint-disable @typescript-eslint/no-unsafe-assignment */
/* eslint-disable @typescript-eslint/no-unsafe-argument */

import preprocess from 'svelte-preprocess'
import adapter from '@sveltejs/adapter-vercel'
import autoprefixer from 'autoprefixer'

/** @type {import('@sveltejs/kit').Config} */
const config = {
	preprocess: preprocess({
		postcss: {
			plugins: [autoprefixer()]
		},
		scss: {
			prependData: ['styles/colors'].map(path => `@use '${path}';`)
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
					'https://www.google-analytics.com'
				],
				'script-src': [
					'self',
					'https://pagead2.googlesyndication.com',
					'https://www.googletagmanager.com'
				],
				'style-src': ['self', 'unsafe-inline', 'https://fonts.googleapis.com'],
				'img-src': ['self', 'https://pagead2.googlesyndication.com'],
				'font-src': ['self', 'https://fonts.gstatic.com'],
				'frame-src': [
					'self',
					'https://googleads.g.doubleclick.net',
					'https://tpc.googlesyndication.com',
					'https://www.google.com'
				]
			}
		},
		vite: {
			server: {
				fs: {
					allow: ['.']
				}
			},
			build: {
				assetsInlineLimit: 0
			}
		}
	}
}

export default config
