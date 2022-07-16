import { getFirestore } from 'firebase-admin/firestore'

import type CommunityLevel from '..'
import admin from '$lib/firebase/admin'

const firestore = getFirestore(admin)

const getAllCommunityLevels = async () =>
	(await firestore.collection('community_levels').get()).docs.map(
		snapshot => ({ ...snapshot.data(), id: snapshot.id } as CommunityLevel)
	)

export default getAllCommunityLevels
