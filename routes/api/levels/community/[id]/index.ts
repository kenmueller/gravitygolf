import type { RequestHandler } from '@sveltejs/kit'
import { getFirestore } from 'firebase-admin/firestore'

import admin from '$lib/firebase/admin'
import communityLevelFromSnapshot from '$lib/level/community/snapshot/from'
import HttpError from '$lib/error/http'
import ErrorCode from '$lib/error/code'
import errorFromValue from '$lib/error/from/value'

const firestore = getFirestore(admin)

export const GET: RequestHandler = async ({ params: { id } }) => {
	const headers = {
		'access-control-allow-methods': 'GET',
		'access-control-allow-origin': '*',
		'access-control-allow-credentials': 'true'
	}

	try {
		const level = communityLevelFromSnapshot(
			await firestore.doc(`community_levels/${id}`).get()
		)

		if (!level)
			throw new HttpError(ErrorCode.NotFound, 'Community level not found')

		return {
			headers: { ...headers, 'content-type': 'application/json' },
			body: JSON.stringify(level)
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { headers, status: code, body: message }
	}
}
