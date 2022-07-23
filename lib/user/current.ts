import type { Readable, Writable, Unsubscriber } from 'svelte/store'
import { derived } from 'svelte/store'
import { getAuth, onAuthStateChanged } from 'firebase/auth'
import { getFirestore, doc, onSnapshot } from 'firebase/firestore'

import { browser } from '$app/env'
import { session } from '$app/stores'

import type Session from '$lib/session'
import type User from '.'
import app from '$lib/firebase'
import sendToken from '$lib/token/send'
import userFromAuth from './from/auth'
import userFromSnapshot from './from/snapshot'

const auth = getAuth(app)
const firestore = getFirestore(app)

const currentUser: Readable<User | null> = derived(
	session as Writable<Session>,
	($session, set) => {
		set($session.user)
		if (!browser) return

		let valid = true
		let snapshotUnsubscribe: Unsubscriber | null = null

		const authUnsubscribe: Unsubscriber = onAuthStateChanged(
			auth,
			user => {
				if (!valid) return

				snapshotUnsubscribe?.()
				snapshotUnsubscribe = null

				sendToken(user).catch(console.error)

				if (!user) return set(null)

				set(userFromAuth(user))

				snapshotUnsubscribe = onSnapshot(
					doc(firestore, `users/${user.uid}`),
					snapshot => set(userFromSnapshot(snapshot) ?? userFromAuth(user)),
					console.error
				)
			},
			console.error
		)

		return () => {
			valid = false

			authUnsubscribe()
			snapshotUnsubscribe?.()
		}
	}
)

export default currentUser
