import type { RequestHandler } from '@sveltejs/kit'
import { getFirestore } from 'firebase-admin/firestore'

import admin from '$lib/admin'
import HttpError from '$lib/error'
import ErrorCode from '$lib/error/code'
import errorFromValue from '$lib/error/from/value'

const firestore = getFirestore(admin)

export const get: RequestHandler = async ({ params: { id } }) => {
	try {
		const snapshot = await firestore.doc(`community_levels/${id}`).get()

		if (!snapshot.exists)
			throw new HttpError(ErrorCode.NotFound, 'Community level not found')

		return {
			headers: { 'content-type': 'application/json' },
			body: JSON.stringify({ ...snapshot.data(), id: snapshot.id })
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { status: code, body: message }
	}
}
