import type Fetch from '$lib/fetch'
import type CommunityLevel from '.'
import prefixUrl from '$lib/url/prefix'
import ErrorCode from '$lib/error/code'
import errorFromResponse from '$lib/error/from/response'

const getCommunityLevel = async (fetch: Fetch, id: string) => {
	const response = await fetch(
		prefixUrl(`/api/levels/community/${encodeURIComponent(id)}`)
	)

	if (response.status === ErrorCode.NotFound) return null
	if (!response.ok) throw await errorFromResponse(response)

	return (await response.json()) as CommunityLevel
}

export default getCommunityLevel
