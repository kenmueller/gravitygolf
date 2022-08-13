import type Fetch from '$lib/fetch'
import type CommunityLevelRecord from '../record'
import prefixUrl from '$lib/url/prefix'
import errorFromResponse from '$lib/error/from/response'

const getCommunityLevels = async (fetch: Fetch, query: string) => {
	const response = await fetch(
		prefixUrl(`/api/levels/community?query=${encodeURIComponent(query)}`)
	)

	if (!response.ok) throw await errorFromResponse(response)

	return (await response.json()) as CommunityLevelRecord[]
}

export default getCommunityLevels
