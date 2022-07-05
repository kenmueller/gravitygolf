import type { ServiceAccount } from 'firebase-admin/app'
import { initializeApp, cert } from 'firebase-admin/app'

const admin = initializeApp({
	storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
	credential: cert(
		JSON.parse(
			Buffer.from(import.meta.env.VITE_FIREBASE_ADMIN_KEY, 'base64').toString()
		) as ServiceAccount
	)
})

export default admin
