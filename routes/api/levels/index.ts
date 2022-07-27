import type { RequestHandler } from '@sveltejs/kit'
import { getFirestore } from 'firebase-admin/firestore'

import admin from '$lib/firebase/admin'
import mainLevelFromSnapshot from '$lib/level/main/snapshot/from'
import errorFromValue from '$lib/error/from/value'

const firestore = getFirestore(admin)

export const GET: RequestHandler = async () => {
	try {
		const { docs } = await firestore.collection('levels').orderBy('index').get()

		return {
			headers: { 'content-type': 'application/json' },
			body: JSON.stringify(docs.map(mainLevelFromSnapshot).filter(Boolean))
		}
	} catch (value) {
		const { code, message } = errorFromValue(value)
		return { status: code, body: message }
	}
}
