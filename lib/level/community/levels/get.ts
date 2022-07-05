import type Fetch from '$lib/fetch'
import type CommunityLevelRecord from '../record'

const getCommunityLevels = async (fetch: Fetch, query: string) => {
	const response = await fetch(
		`/communityLevels?query=${encodeURIComponent(query)}`
	)
	if (!response.ok) throw new Error(await response.text())

	return (await response.json()) as CommunityLevelRecord[]
}

export default getCommunityLevels
