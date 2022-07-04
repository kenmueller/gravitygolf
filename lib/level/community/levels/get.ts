import type CommunityLevelRecord from '../record'

type Fetch = (info: RequestInfo, init?: RequestInit) => Promise<Response>

const getCommunityLevels = async (
	fetch: Fetch = window.fetch,
	query: string
) => {
	const response = await fetch(
		`/community_levels?query=${encodeURIComponent(query)}`
	)
	if (!response.ok) throw new Error(await response.text())

	return (await response.json()) as CommunityLevelRecord[]
}

export default getCommunityLevels
