import type MainLevel from '..'
import type RawLevel from '$lib/level/raw'
import type Snapshot from '$lib/snapshot'
import exists from '$lib/snapshot/exists'
import get from '$lib/snapshot/get'

const mainLevelFromSnapshot = (snapshot: Snapshot) => {
	if (!exists(snapshot)) return null

	const data = get<string, null>(snapshot, 'data', 'string', null)
	if (!data) return null

	return {
		id: snapshot.id,
		message: get(snapshot, 'message', 'string', null),
		data: JSON.parse(data) as RawLevel
	} as MainLevel
}

export default mainLevelFromSnapshot
