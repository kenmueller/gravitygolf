import type { RequestHandler } from '@sveltejs/kit'
import type { WebAppManifest } from 'web-app-manifest'

import errorFromValue from '$lib/error/from/value'

import touch from '../images/touch.png'
import mask from '../images/mask.png'

const manifest: WebAppManifest = {
	dir: 'ltr',
	lang: 'en-US',
	scope: '/',
	start_url: '/',
	name: 'Gravity Golf',
	short_name: 'Gravity Golf',
	description: 'The game that combines golf, gravity, and antigravity',
	display: 'standalone',
	theme_color: '#24292e',
	background_color: '#24292e',
	categories: ['games', 'entertainment', 'education'],
	icons: [
		{ src: touch, sizes: '180x180' },
		{ src: mask, sizes: '512x512', purpose: 'monochrome maskable' }
	]
}

let data: string | null = null

export const get: RequestHandler = () => {
	try {
		return {
			headers: {
				'cache-control': 'no-cache',
				'content-type': 'application/manifest+json'
			},
			body: (data ??= JSON.stringify(manifest))
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { status: code, body: message }
	}
}
