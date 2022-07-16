import type { FirebaseError } from 'firebase/app'
import { getApp, initializeApp } from 'firebase/app'

const app = (() => {
	try {
		return getApp()
	} catch (error) {
		if ((error as FirebaseError).code !== 'app/no-app') throw error

		return initializeApp({
			apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
			authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
			projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
			storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
			messagingSenderId: import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID,
			appId: import.meta.env.VITE_FIREBASE_APP_ID,
			measurementId: import.meta.env.VITE_FIREBASE_MEASUREMENT_ID
		})
	}
})()

export default app
