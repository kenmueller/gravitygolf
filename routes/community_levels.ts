import type { RequestHandler } from '@sveltejs/kit'

import client from '$lib/level/community/client'
import fromRecord from '$lib/level/community/record/from'
import errorFromValue from '$lib/error/from/value'

export const get: RequestHandler = async ({ url }) => {
	try {
		const query = url.searchParams.get('query') ?? ''
		const { hits } = await client.search(query)

		return {
			headers: { 'content-type': 'application/json' },
			body: JSON.stringify(hits.map(fromRecord))
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { status: code, body: message }
	}
}
