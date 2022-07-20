import type { RequestHandler } from '@sveltejs/kit'
import { getFirestore } from 'firebase-admin/firestore'

import admin from '$lib/firebase/admin'
import client from '$lib/level/community/client'
import fromRecord from '$lib/level/community/record/from'
import validateRawLevel from '$lib/level/raw/validate'
import replaceWithRounded from '$lib/replaceWithRounded'
import HttpError from '$lib/error'
import ErrorCode from '$lib/error/code'
import errorFromValue from '$lib/error/from/value'

const firestore = getFirestore(admin)

export const GET: RequestHandler = async ({ url }) => {
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

export const POST: RequestHandler = async ({ request }) => {
	try {
		if (request.headers.get('content-type') !== 'application/json')
			throw new HttpError(ErrorCode.BadRequest, 'Content-Type must be JSON')

		const json = (await request.json()) as Record<string, unknown> | null
		if (!json) throw new HttpError(400, 'Invalid JSON')

		const { name, data } = json

		if (!(typeof name === 'string' && name))
			throw new HttpError(400, 'Invalid name')

		if (!validateRawLevel(data))
			throw new HttpError(ErrorCode.BadRequest, 'Invalid data')

		const document = firestore.collection('community_levels').doc()

		await document.create({
			name,
			gravity: Array.isArray(data.gravity)
				? data.gravity[0] + (data.gravity.length - 1)
				: data.gravity ?? 0,
			antigravity: Array.isArray(data.antigravity)
				? data.antigravity[0] + (data.antigravity.length - 1)
				: data.antigravity ?? 0,
			stars: data.stars?.length ?? 0,
			attempts: 0,
			wins: 0,
			data: JSON.stringify(data, replaceWithRounded(2))
		})

		return { body: document.id }
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { status: code, body: message }
	}
}
