import type { RequestHandler } from '@sveltejs/kit'
import { getFirestore } from 'firebase-admin/firestore'

import admin from '$lib/firebase/admin'
import communityLevelFromSnapshot from '$lib/level/community/snapshot/from'
import HttpError from '$lib/error/http'
import ErrorCode from '$lib/error/code'
import errorFromValue from '$lib/error/from/value'

const firestore = getFirestore(admin)

export const GET: RequestHandler = async ({ params: { id } }) => {
	try {
		const level = communityLevelFromSnapshot(
			await firestore.doc(`community_levels/${id}`).get()
		)

		if (!level)
			throw new HttpError(ErrorCode.NotFound, 'Community level not found')

		return {
			headers: { 'content-type': 'application/json' },
			body: JSON.stringify(level)
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { status: code, body: message }
	}
}
