import type { FirebaseError } from 'firebase/app'
import { getAuth, GoogleAuthProvider, signInWithPopup } from 'firebase/auth'

import app from '$lib/firebase'

const auth = getAuth(app)

const provider = new GoogleAuthProvider()
provider.addScope('https://www.googleapis.com/auth/userinfo.email')

const signIn = async () => {
	try {
		await signInWithPopup(auth, provider)
		return true
	} catch (error) {
		switch ((error as FirebaseError).code) {
			case 'auth/popup-closed-by-user':
				return false
			default:
				throw error
		}
	}
}

export default signIn
