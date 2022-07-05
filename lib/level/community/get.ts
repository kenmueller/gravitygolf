import ErrorCode from '$lib/error/code'
import type Fetch from '$lib/fetch'
import type CommunityLevel from '.'

const getCommunityLevel = async (fetch: Fetch, id: string) => {
	const response = await fetch(`/communityLevels/${encodeURIComponent(id)}`)

	if (response.status === ErrorCode.NotFound) return null
	if (!response.ok) throw new Error(await response.text())

	return (await response.json()) as CommunityLevel
}

export default getCommunityLevel
