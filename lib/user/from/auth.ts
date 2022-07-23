import type { User as FirebaseUser } from 'firebase/auth'

import type User from '..'
import nameFromEmail from '$lib/name/from/email'

const userFromAuth = (user: FirebaseUser): User => {
	const { creationTime } = user.metadata

	return {
		id: user.uid,
		name: user.displayName || (user.email && nameFromEmail(user.email)) || null,
		email: user.email || null,
		created: creationTime ? new Date(creationTime) : null
	}
}

export default userFromAuth
