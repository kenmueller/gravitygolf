import type User from '..'
import type Snapshot from '$lib/snapshot'
import get from '$lib/snapshot/get'

const userFromSnapshot = (snapshot: Snapshot): User | null => {
	if (!snapshot.exists) return null

	return {
		id: snapshot.id,
		name: get(snapshot, 'name', 'string', null),
		email: get(snapshot, 'email', 'string', null),
		created: get(snapshot, 'created', 'date', null)
	}
}

export default userFromSnapshot
