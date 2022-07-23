import type { FirebaseError } from 'firebase-admin/app'
import { getAuth } from 'firebase-admin/auth'

import type User from '../user'
import app from '../firebase/admin'
import nameFromEmail from '$lib/name/from/email'

const auth = getAuth(app)

/** Gets user data only from auth data. */
const initialUserFromToken = async (token: string): Promise<User | null> => {
	try {
		const user = await auth.verifyIdToken(token)

		return {
			id: user.uid,
			name:
				(user.name as string | null | undefined) ||
				(user.email && nameFromEmail(user.email)) ||
				null,
			email: user.email || null,
			created: null
		}
	} catch (error) {
		switch ((error as FirebaseError)?.code) {
			case 'auth/argument-error':
			case 'auth/id-token-expired':
				return null
			default:
				throw error
		}
	}
}

export default initialUserFromToken
