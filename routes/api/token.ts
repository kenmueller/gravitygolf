import type { RequestHandler } from '@sveltejs/kit'

import idFromToken from '$lib/token/user'
import setToken from '$lib/token/set'
import ErrorCode from '$lib/error/code'
import HttpError from '$lib/error/http'
import errorFromValue from '$lib/error/from/value'

export const OPTIONS: RequestHandler = () => ({
	headers: {
		'access-control-allow-methods': 'OPTIONS, POST',
		'access-control-allow-origin': '*',
		'access-control-allow-credentials': 'true'
	},
	body: ''
})

export const POST: RequestHandler = async ({ request }) => {
	const headers = {
		'access-control-allow-methods': 'OPTIONS, POST',
		'access-control-allow-origin': '*',
		'access-control-allow-credentials': 'true'
	}

	try {
		if (request.headers.get('content-type') !== 'application/json')
			throw new HttpError(400, 'Invalid content type')

		const token = (await request.json()) as string | null

		if (!((typeof token === 'string' && token) || token === null))
			throw new HttpError(ErrorCode.BadRequest, 'Invalid token')

		if (token)
			try {
				if (!(await idFromToken(token))) throw new Error()
			} catch {
				throw new HttpError(ErrorCode.Forbidden, 'Invalid token')
			}

		return {
			headers: { ...headers, 'set-cookie': setToken(token) },
			body: ''
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { headers, status: code, body: message }
	}
}
