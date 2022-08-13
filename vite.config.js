import { sveltekit } from '@sveltejs/kit/vite'

/** @type {import('vite').UserConfig} */
const config = {
	plugins: [sveltekit()],
	define: {
		MOBILE: Boolean(process.env.MOBILE)
	},
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
