import type { RequestHandler } from '@sveltejs/kit'
import { getFirestore, FieldValue } from 'firebase-admin/firestore'

import type Property from '../property'
import admin from '$lib/firebase/admin'
import ErrorCode from '$lib/error/code'
import HttpError from '$lib/error/http'
import errorFromValue from '$lib/error/from/value'

const firestore = getFirestore(admin)

const incrementCommunityLevelPropertyHandler =
	(property: Property): RequestHandler =>
	async ({ params: { id } }) => {
		const headers = {
			'access-control-allow-methods': 'OPTIONS, POST',
			'access-control-allow-origin': '*',
			'access-control-allow-credentials': 'true'
		}

		try {
			await firestore.doc(`community_levels/${id}`).update({
				[property]: FieldValue.increment(1)
			})

			return { headers, body: '' }
		} catch (value) {
			const { code, message } =
				(value as { code: number }).code === 5
					? new HttpError(ErrorCode.NotFound, 'Community level not found')
					: errorFromValue(value)

			return { headers, status: code, body: message }
		}
	}

export default incrementCommunityLevelPropertyHandler
