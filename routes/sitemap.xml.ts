import type { RequestHandler } from '@sveltejs/kit'

import levels from '$lib/level/levels'
import errorFromValue from '../lib/error/from/value'
import getAllCommunityLevels from '$lib/level/community/levels/all'

const getUrls = async () => [
	'/',
	'/levels',
	...levels.map((_level, index) => `/levels/${index + 1}`),
	'/levels/community',
	...(await getAllCommunityLevels()).map(
		({ id }) => `/levels/community/${encodeURIComponent(id)}`
	),
	'/levels/editor'
]

const sitemap = async (origin: string) =>
	`<?xml version="1.0" encoding="UTF-8" ?>\
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9" \
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" \
xsi:schemaLocation="http://www.sitemaps.org/schemas/sitemap/0.9 \
http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd">\
${(await getUrls())
	.map(url => `<url><loc>${encodeURI(new URL(url, origin).href)}</loc></url>`)
	.join('')}\
</urlset>`

let data: string | null = null

export const get: RequestHandler = async ({ url }) => {
	try {
		return {
			headers: {
				'cache-control': 'no-cache',
				'content-type': 'application/xml'
			},
			body: (data ??= await sitemap(url.origin))
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { status: code, body: message }
	}
}
