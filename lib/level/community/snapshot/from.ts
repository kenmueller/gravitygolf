import type CommunityLevel from '..'
import type RawLevel from '$lib/level/raw'
import type Snapshot from '$lib/snapshot'
import exists from '$lib/snapshot/exists'

const communityLevelFromSnapshot = (snapshot: Snapshot) => {
	if (!exists(snapshot)) return null

	const data = snapshot.data() ?? {}

	return {
		...data,
		id: snapshot.id,
		data: JSON.parse(data.data as string) as RawLevel
	} as CommunityLevel
}

export default communityLevelFromSnapshot
