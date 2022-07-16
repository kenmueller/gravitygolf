import type { FirebaseError, ServiceAccount } from 'firebase-admin/app'
import { getApp, initializeApp, cert } from 'firebase-admin/app'

const admin = (() => {
	try {
		return getApp()
	} catch (error) {
		if ((error as FirebaseError).code !== 'app/no-app') throw error

		return initializeApp({
			storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
			credential: cert(
				JSON.parse(
					Buffer.from(
						import.meta.env.VITE_FIREBASE_ADMIN_KEY,
						'base64'
					).toString()
				) as ServiceAccount
			)
		})
	}
})()

export default admin
