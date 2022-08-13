import type { RequestHandler } from '@sveltejs/kit'

import levels from '$lib/level/levels'
import errorFromValue from '$lib/error/from/value'
import getAllCommunityLevels from '$lib/level/community/levels/all'

const getUrls = async () => [
	'/',
	'/levels',
	...levels.map(
		(_level, index) =>
			`/levels/${MOBILE ? 'id?value=' : ''}${encodeURIComponent(index + 1)}`
	),
	`/levels/community${MOBILE ? '/mobile' : ''}`,
	...(await getAllCommunityLevels()).map(
		({ id }) =>
			`/levels/community/${MOBILE ? 'id?value=' : ''}${encodeURIComponent(id)}`
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

export const GET: RequestHandler = async ({ url }) => {
	const headers = {
		'access-control-allow-methods': 'GET',
		'access-control-allow-origin': '*',
		'access-control-allow-credentials': 'true'
	}

	try {
		return {
			headers: {
				...headers,
				'cache-control': 'no-cache',
				'content-type': 'application/xml'
			},
			body: (data ??= await sitemap(url.origin))
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { headers, status: code, body: message }
	}
}
