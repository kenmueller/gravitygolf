import { sveltekit } from '@sveltejs/kit/vite'

const plugins = [sveltekit()]

/** @type {import('vite').UserConfig} */
const config = {
	plugins,
	server: {
		fs: {
			allow: ['.']
		}
	},
	build: {
		assetsInlineLimit: 0
	}
}

export default config
