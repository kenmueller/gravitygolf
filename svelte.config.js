/* eslint-disable @typescript-eslint/no-unsafe-assignment */
/* eslint-disable @typescript-eslint/no-unsafe-argument */

import preprocess from 'svelte-preprocess'
import adapter from '@sveltejs/adapter-vercel'

/** @type {import('@sveltejs/kit').Config} */
const config = {
	preprocess: preprocess(),
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
			serviceWorker: 'lib/service-worker',
			template: 'lib/index.html'
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
